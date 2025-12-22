#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uAlbedo;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 ambient = 0.15 * uAlbedo;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uAlbedo * uLightColor;

    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = spec * uLightColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
