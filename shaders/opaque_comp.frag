#version 450

struct MaterialData {
    vec4 baseColor;
    float thickness;
    float ior;
    float metallic;
    float roughness;
    mat4 cubeInv;
};

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform CameraData {
    mat4 viewProj;
    vec3 viewPos;
} camera;
layout(binding = 2) uniform LightData {
    vec3 lightPos;
    vec3 lightColor;
} lightData;
layout(binding = 3) uniform Materials {
    MaterialData data[512];
} materials;
layout(binding = 4) uniform sampler2D sceneSampler;
layout(binding = 5) uniform samplerCube envSampler;
layout(binding = 8) uniform sampler2D ssrResolve;
layout(binding = 11) uniform ProbeData {
    vec4 probe;
    uint count;
} probeData;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) flat in vec3 viewPos;
layout(location = 4) flat in uint materialIdx;

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
        mat4 cubeInv = materials.data[i].cubeInv;
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
        if (tmin > 1e-4 && tmin < tmax && tmin < bestT) {
            bestT = tmin;
        }
    }
    if (bestT < 1e29) {
        return texture(envSampler, (P + R * bestT - probeData.probe.xyz)).rgb;
    }
    return skyColor(R);
}

void main() {
    vec4 sampled = texture(texSampler, fragUV);
    vec4 base = sampled * materials.data[materialIdx].baseColor;

    if (base.a < 0.5) discard;

    vec3 baseRgb = base.rgb;
    vec3 N = normalize(fragNormal);
    vec3 lightDir = normalize(lightData.lightPos - fragPos);
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = diff * lightData.lightColor;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightData.lightColor;

    float ambient = 0.10;
    vec3 lit = (ambient + diffuse + specular) * baseRgb;

    vec3 V = normalize(fragPos - viewPos);
    float cosI = clamp(dot(-V, N), 0.0, 1.0);
    float F = 0.04 + 0.96 * pow(1.0 - cosI, 5.0);

    ivec2 ssrSize = textureSize(ssrResolve, 0);
    vec2 suv = gl_FragCoord.xy / vec2(ssrSize);
    vec4 ssr = texture(ssrResolve, suv);
    vec3 refl;
    float w;
    if (ssr.z > 0.5) {
        refl = texture(sceneSampler, ssr.xy).rgb;
        w = ssr.w;
    } else {
        vec3 R = reflect(V, N);
        refl = fallbackReflection(fragPos, R);
        w = 1.0;
    }

    vec3 color = mix(lit, refl, clamp(F * w, 0.0, 1.0));
    outColor = vec4(color, 1.0);
}
