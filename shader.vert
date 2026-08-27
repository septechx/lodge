#version 450

layout(push_constant) uniform ModelMatrix {
    mat4 model;
} model;

layout(binding = 1) uniform CameraData {
    mat4 viewproj;
} camera;

layout(location = 0) in vec3 pos;

layout(location = 0) out vec2 fragUV;

void main() {
    fragUV = vec2(1.0f, 1.0f);
    gl_Position = camera.viewproj * model.model * vec4(pos, 1.0);
}
