#version 450

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 2) uniform LightData {
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
} lightData;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 sampled = texture(texSampler, fragUV);
    vec4 base = sampled * fragColor;

    if (base.a < 0.5) discard;

    vec3 baseRgb = base.rgb;

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightData.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightData.lightColor;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(lightData.viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightData.lightColor;

    float ambient = 0.10;
    vec3 result = (ambient + diffuse + specular) * baseRgb;

    outColor = vec4(result, base.a);
}
