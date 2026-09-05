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

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 0, binding = 1) uniform LightData {
    vec3 lightPos;
    vec3 lightColor;
} lightData;
layout(set = 0, binding = 3) uniform Objects {
    ObjectData data[512];
} objects;
layout(set = 1, binding = 1) uniform Materials {
    MaterialData data[512];
} materials;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) flat in vec3 viewPos;
layout(location = 4) flat in uint objectIdx;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

void main() {
    vec4 sampled = texture(texSampler, fragUV);
    ObjectData obj = objects.data[objectIdx];
    vec4 base = sampled * materials.data[obj.materialId].baseColor;

    if (base.a < 0.5) discard;

    vec3 baseRgb = base.rgb;

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightData.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightData.lightColor;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightData.lightColor;

    float ambient = 0.10;
    vec3 result = (ambient + diffuse + specular) * baseRgb;

    outColor = vec4(result, base.a);
    outNormal = vec4(norm * 0.5 + 0.5, 1.0);
}
