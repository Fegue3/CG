#version 330 core
in vec2 vUV;

uniform vec3 uAlbedo;
uniform float uAlpha;

uniform int uUseTex;
uniform sampler2D uTex;

uniform int uUseMask;
uniform vec2 uMaskMin;
uniform vec2 uMaskMax;

out vec4 FragColor;

void main() {
    vec3 color = uAlbedo;
    float alpha = uAlpha;
    
    if (uUseTex == 1) {
        vec4 texColor = texture(uTex, vUV);
        color *= texColor.rgb;
        alpha *= texColor.a;
    }
    
    // Screen-space rectangular masking
    if (uUseMask == 1) {
        if (gl_FragCoord.x >= uMaskMin.x && gl_FragCoord.x <= uMaskMax.x &&
            gl_FragCoord.y >= uMaskMin.y && gl_FragCoord.y <= uMaskMax.y) {
            alpha = 0.0;
        }
    }
    
    FragColor = vec4(color, alpha);
}
