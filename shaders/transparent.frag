#version 450

struct MaterialData {
    vec4 baseColor;
    float thickness;
    float ior;
    float _pad0;
    float _pad1;
};

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 2) uniform LightData {
    vec3 lightPos;
    vec3 lightColor;
} lightData;
layout(binding = 3) uniform Materials {
    MaterialData data[512];
} materials;
layout(binding = 4) uniform sampler2D sceneSampler;
layout(binding = 5) uniform samplerCube envSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) flat in vec3 viewPos;
layout(location = 4) flat in uint materialIdx;
layout(location = 5) flat in mat4 viewProj;

layout(location = 0) out vec4 outColor;

void main() {
    MaterialData material = materials.data[materialIdx];

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(fragPos - viewPos);
    float cosI = clamp(dot(-V, N), 0.0, 1.0);

    float eta = 1.0 / material.ior;
    vec3 T = refract(V, N, eta);
    vec3 exitPoint = fragPos + T * material.thickness;
    vec4 clip = viewProj * vec4(exitPoint, 1.0);
    vec2 exitUv = (clip.xy / clip.w) * 0.5 + 0.5;
    vec3 transmitted = texture(sceneSampler, exitUv).rgb;

    vec3 sigma = clamp(1.0 - material.baseColor.rgb, 0.0, 1.0) * 8.0;
    vec3 absorb = exp(-sigma * material.thickness);
    transmitted *= absorb;

    vec3 R = reflect(V, N);
    vec3 reflected = texture(envSampler, R).rgb;
    float F0 = pow((material.ior - 1.0) / (material.ior + 1.0), 2.0);
    float F = F0 + (1.0 - F0) * pow(1.0 - cosI, 5.0);

    vec3 color = mix(transmitted, reflected, F);

    vec3 L = normalize(vec3(0.6, 0.9, 0.3));
    float shade = 0.9 + 0.1 * max(dot(N, L), 0.0);
    color *= shade;

    outColor = vec4(color, 1.0);
}
