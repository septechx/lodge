#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint materialIdx;
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
layout(location = 3) flat out vec3 viewPos;
layout(location = 4) flat out uint materialIdx;
layout(location = 5) flat out mat4 viewProj;

void main() {
    fragUV = inUv;
    viewPos = camera.viewPos;
    materialIdx = pc.materialIdx;
    viewProj = camera.viewProj;

    mat3 normalMat = transpose(inverse(mat3(pc.model)));
    fragNormal = normalize(normalMat * inNormal);

    fragPos = vec3(pc.model * vec4(inPos, 1.0));

    gl_Position = camera.viewProj * pc.model * vec4(inPos, 1.0);
}
