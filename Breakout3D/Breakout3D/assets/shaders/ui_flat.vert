#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;  // Defined but unused for UI
layout(location = 2) in vec2 aUV;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

out vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = uP * uV * uM * vec4(aPos, 1.0);
}
