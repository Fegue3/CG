#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 world = uM * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(uM))) * aNormal;
    gl_Position = uP * uV * world;
}
