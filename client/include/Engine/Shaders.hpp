#pragma once

#include <glad/glad.h>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include <glm/glm.hpp>

class Shader {
    public:

        Shader();
        Shader(const std::filesystem::path& vertexShader, const std::filesystem::path& fragmentShader);
        void initialize(const std::filesystem::path& vertexShader, const std::filesystem::path& fragmentShader);
        void use() const {
            glUseProgram(ID);
        }
        unsigned int getID() const {
            return ID;
        }
        void setBool(const std::string &name, bool value) const;
        void setInt(const std::string &name, int value) const;
        void setFloat(const std::string &name, float value) const;
        void setVec3(const std::string &name, glm::vec3 value) const;
        void setVec4(const std::string &name, glm::vec4 value) const;
        void setMat3(const std::string &name, glm::mat3 value) const;
        void setMat4(const std::string &name, glm::mat4 value) const;

    private:
        unsigned int ID;
};