#version 450

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 2) uniform LightData {
    vec3 lightPos;
    vec3 lightColor;
} lightData;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec4 fragColor;
layout(location = 4) in vec3 viewPos;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragUV) * fragColor;
}
