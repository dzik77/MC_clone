#version 460 core

layout (location = 0) in uint packedData;
out vec2 TexCoords;
out float TexLayer;
out float AmbientOcclusion;

struct ChunkPosition {
    vec4 position;
};

layout(std430, binding = 0) readonly buffer ChunkPositions {
    ChunkPosition positions[];
};

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
    uint y = (packedData >> 5u) & 31u;
    uint z = (packedData >> 10u) & 31u;
    uint textureID = (packedData >> 15u) & 255u;
    uint direction = (packedData >> 23u) & 7u;
    uint uvCoordinate = (packedData >> 26u) & 3u;
    uint ambientOcclusion = (packedData >> 28u) & 3u;

    vec3 finalPos = vec3(float(x), float(y), float(z)) + positions[gl_BaseInstance].position.xyz;

    gl_Position = projection * view * vec4(finalPos, 1.0);

    TexCoords = UV_MAP[uvCoordinate];

    TexLayer = float(int(textureID)-1);

    AmbientOcclusion = float(ambientOcclusion);

}