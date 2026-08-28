#version 450

layout(push_constant) uniform ModelMatrix {
    mat4 model;
} model;
layout(binding = 1) uniform CameraData {
    mat4 viewproj;
} camera;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

void main() {
    fragUV = inUv;

    mat3 normalMat = mat3(model.model);
    fragNormal = normalize(normalMat * inNormal);

    fragPos = vec3(model.model * vec4(inPos, 1.0));

    gl_Position = camera.viewproj * model.model * vec4(inPos, 1.0);
}
