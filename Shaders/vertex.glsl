#version 330 core

layout (location = 0) in uint packedData;
out vec2 TexCoords;
out float TexLayer;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const vec2 UV_MAP[4] = vec2[4](
    vec2(0.0, 1.0), // Index 0 -> Top-Left
    vec2(1.0, 1.0), // Index 1 -> Top-Right
    vec2(1.0, 0.0), // Index 2 -> Bottom-Right
    vec2(0.0, 0.0)  // Index 3 -> Bottom-Left
);

void main() {

    uint x = packedData & 31u;
    uint y = (packedData >> 5u) & 255u;
    uint z = (packedData >> 13u) & 31u;
    uint textureID = (packedData >> 18u) & 255u;
    uint direction = (packedData >> 26u) & 7u;

    gl_Position = projection * view * model * vec4(float(x), float(y), float(z), 1.0);

    TexCoords = UV_MAP[gl_VertexID % 4];

    TexLayer = float(int(textureID)-1);
}