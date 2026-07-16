#include <Engine/Shaders.hpp>
#include <iostream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader() : ID(0) {
    // Default constructor does not initialize the shader program.
    // Use the initialize method to set up the shader program later.
}

Shader::Shader(const std::filesystem::path& vertexShader, const std::filesystem::path& fragmentShader) {
    
    auto readFile = [](const std::filesystem::path& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open shader file: " + path.string());
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };

    std::string vertexCode, fragmentCode;
    try {
        vertexCode = readFile(vertexShader);
        fragmentCode = readFile(fragmentShader);
    } catch (const std::exception& e) {
        std::cerr << "Shader Loading Error: " << e.what() << std::endl;
        throw;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[2048];

    std::cout << "Compiling vertex shader: " << vertexShader << std::endl;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 2048, nullptr, infoLog);
        std::cerr << "Vertex Shader Error (" << vertexShader << "):\n" << infoLog << std::endl;
        throw std::runtime_error("Vertex shader compilation failed.");
    }

    std::cout << "Compiling fragment shader: " << fragmentShader << std::endl;
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 2048, nullptr, infoLog);
        std::cerr << "Fragment Shader Error (" << fragmentShader << "):\n" << infoLog << std::endl;
        throw std::runtime_error("Fragment shader compilation failed.");
    }

    std::cout << "Linking shader program..." << std::endl;
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 2048, nullptr, infoLog);
        std::cerr << "Shader Program Link Error:\n" << infoLog << std::endl;
        throw std::runtime_error("Shader program linking failed.");
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    std::cout << "Shader program linked successfully. ID: " << ID << std::endl;
}

void Shader::initialize(const std::filesystem::path& vertexShader, const std::filesystem::path& fragmentShader) {
    // This function can be used to reinitialize the shader program if needed.
    // For now, it simply calls the constructor logic again.
    *this = Shader(vertexShader, fragmentShader);
}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec3(const std::string &name, glm::vec3 value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string &name, glm::vec4 value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat3(const std::string &name, glm::mat3 value) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, glm::mat4 value) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}
