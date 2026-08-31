#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat3 normal;
    vec4 baseColor;
} pc;
layout(binding = 1) uniform CameraData {
    mat4 viewProj;
    vec3 viewPos;
} camera;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec4 fragColor;
layout(location = 4) out vec3 viewPos;

void main() {
    fragUV = inUv;
    fragColor = pc.baseColor;
    viewPos = camera.viewPos;

    fragNormal = normalize(pc.normal * inNormal);

    fragPos = vec3(pc.model * vec4(inPos, 1.0));

    gl_Position = camera.viewProj * pc.model * vec4(inPos, 1.0);
}
