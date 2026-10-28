#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in int packedData;

uniform mat4 model, view, projection;
uniform mat4 partTransforms[6];

void main() {

    int partID = packedData >> 3;
    // part transform is in model space, so it must be applied before model
    vec4 world = model * partTransforms[partID] * vec4(aPos, 1.0);
    gl_Position = projection * view * world;

}
