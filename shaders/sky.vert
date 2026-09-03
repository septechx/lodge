#version 450

layout(binding = 1) uniform CameraData {
    mat4 viewProj;
    vec3 viewPos;
} camera;

layout(location = 0) out vec3 fragRay;

void main() {
    vec2 pos = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    vec4 clipPos = vec4(pos * 2.0 - 1.0, 1.0, 1.0);
    vec4 world = inverse(camera.viewProj) * clipPos;
    fragRay = world.xyz / world.w - camera.viewPos.xyz;
    gl_Position = clipPos;
}
