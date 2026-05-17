#version 330 core

layout (location = 0) in uint packedData;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    uint x = packedData & 15u;
    uint y = (packedData >> 4u) & 15u;
    uint z = (packedData >> 8u) & 255u;
    uint textureID = (packedData >> 16u) & 255u;
    uint direction = (packedData >> 24u) & 7u;
    gl_Position = projection * view * model * vec4(x,y,z,1.0);
}