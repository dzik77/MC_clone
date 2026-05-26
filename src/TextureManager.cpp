#include "TextureManager.hpp"
#include <cmath>

void TextureArray::createTextureArray(const std::vector<fs::path> texturePaths, int tileWidth, int tileHeight) {

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &texture);

    int layers = texturePaths.size();
    int mipLevels = 1 + static_cast<int>(std::floor(std::log2(std::max(tileWidth, tileHeight))));

    glTextureStorage3D(texture, mipLevels, GL_RGBA8, tileWidth, tileHeight, layers);

    for (int i = 0; i < layers; i++){

        if (!fs::exists(texturePaths[i]))
            throw std::runtime_error("Can't find texture file: " + texturePaths[i].string());

        int width, height, channels;
        unsigned char * data = stbi_load(texturePaths[i].string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data)
            throw std::runtime_error("Failed to load texture: " + texturePaths[i].string());

        glTextureSubImage3D(texture, 0, 0, 0, i, tileWidth, tileHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

    }

    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateTextureMipmap(texture);

}