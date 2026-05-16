#pragma once

#include <glad/glad.h>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>

#define fs std::filesystem

class Shader {
    public:

        Shader(const fs::path& vertexShader, const fs::path& fragmentShader);
        void use() const {
            glUseProgram(ID);
        }
        void setBool(const std::string &name, bool value) const;
        void setInt(const std::string &name, int value) const;
        void setFloat(const std::string &name, float value) const;

    private:
        unsigned int ID;
};