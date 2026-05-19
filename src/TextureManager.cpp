#include "TextureManager.hpp"

void TextureArray::createTextureArray(const std::vector<fs::path> texturePaths, int tileWidth, int tileHeight) {

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

    int layers = texturePaths.size();

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, tileWidth, tileHeight, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (int i = 0; i < layers; i++){

        if (!fs::exists(texturePaths[i]))
            throw std::runtime_error("Can't find texture file: " + texturePaths[i].string());

        int width, height, channels;
        unsigned char * data = stbi_load(texturePaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data)
            throw std::runtime_error("Failed to load texture: " + texturePaths[i].string());
        
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0, 0, 0, i, tileWidth, tileHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

}