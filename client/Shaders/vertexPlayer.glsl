#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in int packedData;

uniform mat4 model, view, projection;

void main() {

    int partID = packedData & 7;
    vec4 world = model * vec4(aPos, 1.0);
    gl_Position = projection * view * world;

}