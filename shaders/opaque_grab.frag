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

layout(set = 1, binding = 0) uniform sampler2D baseTex;
layout(set = 1, binding = 1) uniform sampler2D mrTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 0, binding = 1) uniform LightData {
    vec3 lightPos;
    vec3 lightColor;
} lightData;
layout(set = 0, binding = 3) uniform Objects {
    ObjectData data[512];
} objects;
layout(set = 1, binding = 3) uniform Materials {
    MaterialData data[512];
} materials;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) flat in vec3 viewPos;
layout(location = 4) flat in uint objectIdx;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

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
    vec3 norm = perturbNormal(fragUV, normalize(fragNormal), fragPos, mapN);

    vec3 albedo = base.rgb;
    vec3 diffuseCol = albedo * (1.0 - metallic);

    vec3 lightDir = normalize(lightData.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightData.lightColor;

    float specularStrength = 0.5 * (1.0 - roughness);
    float specPow = mix(128.0, 16.0, roughness);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), specPow);
    vec3 specTint = mix(vec3(1.0), albedo, metallic);
    vec3 specular = specularStrength * spec * lightData.lightColor * specTint;

    float ambient = 0.10;
    vec3 result = ambient * diffuseCol + diffuse * diffuseCol + specular;

    outColor = vec4(result, base.a);
    outNormal = vec4(norm * 0.5 + 0.5, 1.0);
}
