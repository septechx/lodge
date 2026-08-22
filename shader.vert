#version 450

layout(push_constant) uniform pc { float t; };

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 fragUV;

void main() {
    mat2 rot = mat2(cos(t), -sin(t),
                    sin(t),  cos(t));
    vec2 p = rot * pos;

    p *= 1.0 + 0.25 * sin(2 * t);

    gl_Position = vec4(p, 0.0, 1.0);

    fragUV = uv;
}
