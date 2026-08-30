#version 450

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragUV) * fragColor;
}
