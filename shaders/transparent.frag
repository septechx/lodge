#version 450

struct MaterialData {
    vec4 baseColor;
    float thickness;
    float ior;
    float _pad0;
    float _pad1;
};

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 2) uniform LightData {
    vec3 lightPos;
    vec3 lightColor;
} lightData;
layout(binding = 3) uniform Materials {
    MaterialData data[512];
} materials;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) flat in vec3 viewPos;
layout(location = 4) flat in uint materialIdx;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragUV) * materials.data[materialIdx].baseColor;
}
