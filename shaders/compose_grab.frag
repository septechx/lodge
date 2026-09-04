#version 450

layout(binding = 4) uniform sampler2D sceneSampler;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sceneSampler, fragUV);
}
