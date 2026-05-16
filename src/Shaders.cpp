#include "Shaders.hpp"

Shader::Shader(const fs::path& vertexShader, const fs::path& fragmentShader) {
    
    if (!fs::exists(vertexShader) || !fs::exists(fragmentShader)) {
        throw std::runtime_error("Shader file not found.");
    }

    std::string vertexCode, fragmentCode;
    const char *vShaderCode, *fShaderCode;
    std::ifstream vertexShaderFile(vertexShader), fragmentShaderFile(fragmentShader);

    if (!vertexShaderFile.is_open() || !fragmentShaderFile.is_open()) {
        throw std::runtime_error("Failed to open shader file.");
    }

    std::stringstream buffer;
    buffer << vertexShaderFile.rdbuf();
    vertexCode = buffer.str();
    vShaderCode = vertexCode.c_str();

    buffer.str("");
    buffer.clear();

    buffer << fragmentShaderFile.rdbuf();
    fragmentCode = buffer.str();
    fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
        throw std::runtime_error("Vertex shader compilation failed: " + std::string(infoLog));
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) { 
        glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
        throw std::runtime_error("Fragment shader compilation failed: " + std::string(infoLog));
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, nullptr, infoLog);
        throw std::runtime_error("Shader program linking failed: " + std::string(infoLog));
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
