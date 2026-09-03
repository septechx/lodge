#version 450

layout(location = 0) in vec3 fragRay;

layout(location = 0) out vec4 outColor;

vec3 skyColor(vec3 d) {
    d = normalize(d);
    float h = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 sky = mix(vec3(0.75, 0.80, 0.85), vec3(0.25, 0.45, 0.80), h);
    float sun = pow(max(dot(d, normalize(vec3(0.6, 0.9, 0.3))), 0.0), 512.0);
    return sky + vec3(1.0, 0.95, 0.85) * sun * 2.0;
}

void main() {
  outColor = vec4(skyColor(fragRay), 1.0);
}
