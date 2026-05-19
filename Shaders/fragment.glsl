#version 330 core

in vec2 TexCoords;
in float TexLayer;
out vec4 FragColor;

uniform sampler2DArray textureArray;

void main() {



    vec4 texColor = texture(textureArray, vec3(TexCoords, TexLayer));

    FragColor = texColor;

}
