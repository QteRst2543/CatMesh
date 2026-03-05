 #version 460 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        
        uniform vec3 color;
        uniform vec3 lightPos;

        void main() {
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(normalize(Normal), lightDir), 0.0);
            vec3 result = color * diff; 
            FragColor = vec4(result, 1.0);
        }