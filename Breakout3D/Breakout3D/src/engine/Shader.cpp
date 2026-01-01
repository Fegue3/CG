// Shader.cpp
// -----------------------------------------------------------------------------
// Shader.cpp
//
// Responsabilidade:
//  - Carregar ficheiros GLSL (vertex/fragment) a partir do disco.
//  - Compilar shaders, fazer link do programa e reportar erros para stderr.
//  - Expor um wrapper simples para usar o programa e definir uniforms comuns.
//
// Notas:
//  - A função load() faz destroy() antes de criar um novo programa para evitar leaks.
//  - checkShader/checkProgram imprimem logs completos quando há falhas.
// -----------------------------------------------------------------------------

#include "engine/Shader.hpp"
#include <GL/glew.h>

#include <fstream>
#include <sstream>
#include <iostream>

namespace engine {

static std::string loadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Failed to open shader file: " << path << "\n";
        return {};
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool checkShader(GLuint s, const char* label) {
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, &len, log.data());
        std::cerr << "Shader compile failed (" << label << "):\n" << log << "\n";
        return false;
    }
    return true;
}

static bool checkProgram(GLuint p) {
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, &len, log.data());
        std::cerr << "Program link failed:\n" << log << "\n";
        return false;
    }
    return true;
}

Shader::Shader() {}

Shader::~Shader() {
    destroy();
}

bool Shader::load(const std::string& vertPath, const std::string& fragPath) {
    destroy();

    std::string vs = loadFile(vertPath);
    std::string fs = loadFile(fragPath);
    if (vs.empty() || fs.empty())
        return false;

    const char* vsrc = vs.c_str();
    const char* fsrc = fs.c_str();

    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vsrc, nullptr);
    glCompileShader(v);
    if (!checkShader(v, "vertex")) {
        glDeleteShader(v);
        return false;
    }

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fsrc, nullptr);
    glCompileShader(f);
    if (!checkShader(f, "fragment")) {
        glDeleteShader(v);
        glDeleteShader(f);
        return false;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, v);
    glAttachShader(m_id, f);
    glLinkProgram(m_id);

    glDeleteShader(v);
    glDeleteShader(f);

    if (!checkProgram(m_id)) {
        destroy();
        return false;
    }

    return true;
}

void Shader::use() const {
    glUseProgram(m_id);
}

void Shader::destroy() {
    if (m_id != 0) {
        glDeleteProgram(m_id);
        m_id = 0;
    }
}

unsigned int Shader::id() const {
    return m_id;
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    GLint loc = glGetUniformLocation(m_id, name.c_str());
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& v) const {
    GLint loc = glGetUniformLocation(m_id, name.c_str());
    if (loc >= 0) glUniform3fv(loc, 1, &v[0]);
}

void Shader::setFloat(const std::string& name, float v) const {
    GLint loc = glGetUniformLocation(m_id, name.c_str());
    if (loc >= 0) glUniform1f(loc, v);
}

} // namespace engine
