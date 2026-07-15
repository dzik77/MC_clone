#pragma once
#include <vector>
#include <filesystem>
#include <string>
#include <glad/glad.h>
#include <Engine/stb_image.h>

#define fs std::filesystem

class TextureArray{
    public:
        TextureArray() : texture(0) {};
        void clean(){
            if (texture != 0)
                glDeleteTextures(1, &texture);
            texture = 0;
        }
        ~TextureArray(){
            
        };
        void createTextureArray(const std::vector<fs::path> texturePaths, int tileWidth, int tileHeight);
        unsigned int texture = 0;
    private:
        
};