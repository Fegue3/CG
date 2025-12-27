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
// 0: modulate base RGB with texture RGB (default for meshes/backgrounds)
// 1: treat texture R as an alpha mask (default for font atlases)
uniform int uTexMode;

// UI vs 3D controls
uniform float uAmbientK;
uniform float uDiffuseK;
uniform float uSpecK;
uniform float uShininess;

// não confiar em defaults no shader: o CPU deve setar sempre
uniform float uAlpha;
uniform int   uUseMask;
uniform vec2  uMaskMin;
uniform vec2  uMaskMax;

out vec4 FragColor;

void main() {
    vec3 base = uAlbedo;
    float texMask = 1.0;
    if (uUseTex == 1) {
        vec4 tex = texture(uTex, vUV);
        if (uTexMode == 0) {
            base *= tex.rgb;
        } else if (uTexMode == 1) {
            texMask = tex.r;
        }
    }

    // Ambient serve para UI/background sem luz
    vec3 color = uAmbientK * base;

    // Só calcula lighting se for mesmo necessário (evita NaNs na UI)
    if (uDiffuseK > 0.0001 || uSpecK > 0.0001) {
        vec3 N = normalize(vNormal);
        vec3 L = normalize(uLightPos - vWorldPos);
        vec3 V = normalize(uViewPos - vWorldPos);

        float diff = max(dot(N, L), 0.0);
        color += (uDiffuseK * diff) * base * uLightColor;

        if (uSpecK > 0.0001) {
            vec3 R = reflect(-L, N);
            float spec = pow(max(dot(V, R), 0.0), uShininess);
            color += (uSpecK * spec) * uLightColor;
        }
    }

    float alpha = uAlpha * texMask;
    if (uUseMask == 1) {
        if (gl_FragCoord.x >= uMaskMin.x && gl_FragCoord.x <= uMaskMax.x &&
            gl_FragCoord.y >= uMaskMin.y && gl_FragCoord.y <= uMaskMax.y) {
            alpha = 0.0;
        }
    }

    FragColor = vec4(color, alpha);
}
