#version 460 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;
in vec2 TexCoords;

uniform vec3 color;
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float lightIntensity;
uniform float ambientStrength;
uniform float innerCutoff;
uniform float outerCutoff;
uniform sampler2D shadowMap;
uniform sampler2D textureSampler;
uniform bool hasTexture;

float ShadowCalculation(vec4 fragPosLightSpace) {
    // Перспективное деление
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Преобразование в диапазон [0,1]
    projCoords = projCoords * 0.5 + 0.5;
    
    // Проверка на выход за границы
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }
    
    // Получение глубины из shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    // Улучшенный bias
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normalize(Normal), lightDir)), 0.005);
    
    // PCF (Percentage Closer Filtering) для смягчения теней
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 toLight = lightPos - FragPos;
    float distanceToLight = max(length(toLight), 0.001);
    vec3 lightVector = normalize(toLight);
    vec3 spotDirection = normalize(lightDir);

    float theta = dot(lightVector, spotDirection);
    float epsilon = max(innerCutoff - outerCutoff, 0.0001);
    float spotFactor = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

    float attenuation = 1.0 / (1.0 + 0.09 * distanceToLight + 0.032 * distanceToLight * distanceToLight);
    float diffuse = max(dot(normal, lightVector), 0.0);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightVector, normal);
    float specFactor = max(dot(viewDir, reflectDir), 0.0);
    float specular = pow(specFactor, 32.0);

    float shadow = ShadowCalculation(FragPosLightSpace);
    float lit = diffuse > 0.0 && spotFactor > 0.0 ? 1.0 : 0.0;
    float shadowFactor = lit > 0.0 ? (1.0 - shadow) : 1.0;

    vec3 baseColor = color;
    if (hasTexture) {
        baseColor = texture(textureSampler, TexCoords).rgb;
    }

    vec3 ambient = baseColor * ambientStrength;
    vec3 lighting = (baseColor * diffuse + lightColor * specular * 0.25) * lightIntensity * attenuation * spotFactor * shadowFactor;
    vec3 result = ambient + lighting;

    FragColor = vec4(result, 1.0);
}
