#version 460 core

in vec2 TexCoords;
in float TexLayer;
in float AmbientOcclusion;
out vec4 FragColor;

uniform sampler2DArray textureArray;

void main() {

    float aoMultiplier = 1.0 - (AmbientOcclusion * 0.2); // Adjust the multiplier as needed

    vec4 texColor = texture(textureArray, vec3(TexCoords, TexLayer));

    FragColor = vec4(texColor.rgb * aoMultiplier, texColor.a);

}
