#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;      // Transformă obiectul în lume
uniform mat4 view;       // Transformă lumea față de cameră
uniform mat4 projection; // Creează perspectiva

void main() {
    // Ordinea înmulțirii este esențială: P * V * M
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}