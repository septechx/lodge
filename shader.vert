#version 450

layout(binding = 1) uniform UBO {
    mat4 viewproj;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 fragUV;

void main() {
    fragUV = uv;
    gl_Position = ubo.viewproj * vec4(pos, 1.0);
}
