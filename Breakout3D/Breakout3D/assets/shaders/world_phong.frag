#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uAlbedo;

uniform int uUseTex;
uniform sampler2D uTex;

uniform float uAmbientK;
uniform float uDiffuseK;
uniform float uSpecK;
uniform float uShininess;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 base = uAlbedo;
    if (uUseTex == 1) {
        base *= texture(uTex, vUV).rgb;
    }

    vec3 ambient = uAmbientK * base;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = (uDiffuseK * diff) * base * uLightColor;

    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), uShininess);
    vec3 specular = (uSpecK * spec) * uLightColor;

    // World shader always outputs opaque
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
