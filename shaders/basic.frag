#version 460 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 color;
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float lightIntensity;
uniform float ambientStrength;
uniform float innerCutoff;
uniform float outerCutoff;

void main() {
    vec3 normal = normalize(Normal);
    vec3 toLight = lightPos - FragPos;
    float distanceToLight = max(length(toLight), 0.001);
    vec3 lightVector = normalize(toLight);
    vec3 spotDirection = normalize(-lightDir);

    float theta = dot(lightVector, spotDirection);
    float epsilon = max(innerCutoff - outerCutoff, 0.0001);
    float spotFactor = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

    float attenuation = 1.0 / (1.0 + 0.09 * distanceToLight + 0.032 * distanceToLight * distanceToLight);
    float diffuse = max(dot(normal, lightVector), 0.0);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfDir = normalize(lightVector + viewDir);
    float specular = pow(max(dot(normal, halfDir), 0.0), 32.0);

    vec3 ambient = color * ambientStrength;
    vec3 directLight = (color * diffuse + lightColor * specular * 0.25) * lightColor;
    vec3 result = ambient + directLight * lightIntensity * attenuation * spotFactor;

    FragColor = vec4(result, 1.0);
}
