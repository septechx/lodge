#version 450

struct MaterialData {
    vec4 baseColor;
    float thickness;
    float ior;
    float metallic;
    float roughness;
};

struct ObjectData {
    mat4 cubeInv;
    uint materialId;
};

struct LightData {
    vec3 pos;
    vec3 color;
};

layout(set = 1, binding = 0) uniform sampler2D baseTex;
layout(set = 1, binding = 1) uniform sampler2D mrTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 0, binding = 0) uniform CameraData {
    mat4 viewProj;
    vec3 viewPos;
} camera;
layout(set = 0, binding = 1) uniform Lights {
    uint count;
    float _pad0;
    float _pad1;
    float _pad2;
    LightData data[16];
} lights;
layout(set = 0, binding = 3) uniform Objects {
    ObjectData data[512];
} objects;
layout(set = 1, binding = 3) uniform Materials {
    MaterialData data[512];
} materials;
layout(set = 3, binding = 0) uniform sampler2D sceneSampler;
layout(set = 2, binding = 0) uniform samplerCube envSampler;
layout(set = 3, binding = 3) uniform sampler2D ssrResolve;
layout(set = 2, binding = 1) uniform ProbeData {
    vec4 probe;
    uint count;
} probeData;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) flat in vec3 viewPos;
layout(location = 4) flat in uint objectIdx;

layout(location = 0) out vec4 outColor;

vec3 skyColor(vec3 d) {
    d = normalize(d);
    float h = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 sky = mix(vec3(0.75, 0.80, 0.85), vec3(0.25, 0.45, 0.80), h);
    float sun = pow(max(dot(d, normalize(vec3(0.6, 0.9, 0.3))), 0.0), 512.0);
    return sky + vec3(1.0, 0.95, 0.85) * sun * 2.0;
}

vec3 fallbackReflection(vec3 P, vec3 R) {
    float bestT = 1e30;
    int n = min(int(probeData.count), 16);
    for (int i = 0; i < n; ++i) {
        mat4 cubeInv = objects.data[i].cubeInv;
        vec3 pL = (cubeInv * vec4(P, 1.0)).xyz;
        vec3 rL = (cubeInv * vec4(R, 0.0)).xyz;
        if (abs(rL.x) < 1e-8) rL.x = 1e-8;
        if (abs(rL.y) < 1e-8) rL.y = 1e-8;
        if (abs(rL.z) < 1e-8) rL.z = 1e-8;
        vec3 tA = (-vec3(1.0) - pL) / rL;
        vec3 tB = (vec3(1.0) - pL) / rL;
        vec3 tmin3 = min(tA, tB);
        vec3 tmax3 = max(tA, tB);
        float tmin = max(max(tmin3.x, tmin3.y), tmin3.z);
        float tmax = min(min(tmax3.x, tmax3.y), tmax3.z);
        if (tmin < tmax) {
            float tHit = (tmin > 1e-4) ? tmin : tmax;
            if (tHit > 1e-4 && tHit < bestT) {
                bestT = tHit;
            }
        }
    }
    if (bestT < 1e29) {
        return texture(envSampler, (P + R * bestT - probeData.probe.xyz)).rgb;
    }
    return skyColor(R);
}

vec3 perturbNormal(vec2 uv, vec3 N, vec3 P, vec3 mapN) {
    if (dot(mapN.xy, mapN.xy) < 1e-6) {
        return normalize(N);
    }
    vec3 q0 = dFdx(P);
    vec3 q1 = dFdy(P);
    vec2 st0 = dFdx(uv);
    vec2 st1 = dFdy(uv);
    vec3 S = q0 * st1.t - q1 * st0.t;
    vec3 T = -q0 * st1.s + q1 * st0.s;
    vec3 N2 = normalize(N);
    S = S - N2 * dot(S, N2);
    T = T - N2 * dot(T, N2);
    float lS = length(S);
    float lT = length(T);
    if (lS < 1e-8 || lT < 1e-8) {
        return N2;
    }
    S /= lS;
    T /= lT;
    float det = dot(cross(S, T), N2);
    T *= sign(det + 1e-8);
    mat3 TBN = mat3(S, T, N2);
    return normalize(TBN * normalize(mapN));
}

void main() {
    ObjectData obj = objects.data[objectIdx];
    MaterialData mat = materials.data[obj.materialId];
    vec4 sampled = texture(baseTex, fragUV);
    vec4 base = sampled * mat.baseColor;

    if (base.a < 0.5) discard;

    vec2 mr = texture(mrTex, fragUV).gb;
    float roughness = clamp(mr.x * mat.roughness, 0.04, 1.0);
    float metallic = clamp(mr.y * mat.metallic, 0.0, 1.0);

    vec3 mapN = texture(normalTex, fragUV).rgb * 2.0 - 1.0;
    vec3 N = perturbNormal(fragUV, normalize(fragNormal), fragPos, mapN);

    vec3 albedo = base.rgb;
    vec3 diffuseCol = albedo * (1.0 - metallic);

    vec3 viewDir = normalize(viewPos - fragPos);

    float specularStrength = 0.5 * (1.0 - roughness);
    float specPow = mix(128.0, 16.0, roughness);
    vec3 specTint = mix(vec3(1.0), albedo, metallic);

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    for (uint i = 0; i < lights.count; ++i) {
        LightData light = lights.data[i];

        vec3 lightDir = normalize(light.pos - fragPos);

        float diff = max(dot(N, lightDir), 0.0);
        diffuse += diff * light.color;

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(N, halfwayDir), 0.0), specPow);
        specular += specularStrength * spec * light.color * specTint;
    }

    float ambient = 0.10;
    vec3 lit = ambient * diffuseCol + diffuse * diffuseCol + specular;

    vec3 V = normalize(fragPos - viewPos);
    float cosI = clamp(dot(-V, N), 0.0, 1.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosI, 5.0);

    ivec2 ssrSize = textureSize(ssrResolve, 0);
    vec2 suv = gl_FragCoord.xy / vec2(ssrSize);
    vec4 ssr = texture(ssrResolve, suv);
    vec3 refl;
    float w;
    if (ssr.z > 0.5) {
        refl = texture(sceneSampler, ssr.xy).rgb;
        w = ssr.w * (1.0 - roughness);
    } else {
        vec3 R = reflect(V, N);
        refl = fallbackReflection(fragPos, R);
        w = 1.0 - roughness * 0.9;
    }

    vec3 reflWeight = clamp(F * w, 0.0, 1.0);
    vec3 color = mix(lit, refl, reflWeight);
    outColor = vec4(color, 1.0);
}
