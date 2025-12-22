#include "engine/Shader.hpp"
#include <fstream>
#include <sstream>
#include <string>

static std::string readFile(const char* path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

namespace engine {

bool Shader::load(const char* vertPath, const char* fragPath) {
    std::string vs = readFile(vertPath);
    std::string fs = readFile(fragPath);

    GLuint v = compile(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = compile(GL_FRAGMENT_SHADER, fs.c_str());

    m_program = glCreateProgram();
    glAttachShader(m_program, v);
    glAttachShader(m_program, f);
    glLinkProgram(m_program);

    glDeleteShader(v);
    glDeleteShader(f);

    return m_program != 0;
}

void Shader::destroy() {
    if (m_program) glDeleteProgram(m_program);
    m_program = 0;
}

void Shader::use() const { glUseProgram(m_program); }

} // namespace engine
