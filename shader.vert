#version 450

layout(binding = 1) uniform CameraData {
    mat4 viewproj;
} camera;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 fragUV;

void main() {
    fragUV = uv;
    gl_Position = camera.viewproj * vec4(pos, 1.0);
}
