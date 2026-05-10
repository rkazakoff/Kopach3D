// Обёртка над OpenGL-шейдерной программой.

#include "engine/Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <cstdio>

// ── Конструктор: компилируем оба шейдера и связываем их в программу ──────────
Shader::Shader(const char* vertSrc, const char* fragSrc)
{
    GLuint vert = compile(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compile(GL_FRAGMENT_SHADER, fragSrc);

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);

    // Проверяем успешность линковки
    GLint ok = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        glDeleteProgram(m_id);
        glDeleteShader(vert);
        glDeleteShader(frag);
        throw std::runtime_error(std::string("Shader link error:\n") + log);
    }

    // Шейдеры больше не нужны — удаляем
    glDeleteShader(vert);
    glDeleteShader(frag);
}

Shader::~Shader()
{
    glDeleteProgram(m_id);
}

// Включить / выключить программу
void Shader::bind()   const { glUseProgram(m_id); }
void Shader::unbind() const { glUseProgram(0);    }

// ── Передача uniform-переменных ───────────────────────────────────────────────

void Shader::setInt(const char* name, int v) const
{
    glUniform1i(glGetUniformLocation(m_id, name), v);
}

void Shader::setFloat(const char* name, float v) const
{
    glUniform1f(glGetUniformLocation(m_id, name), v);
}

void Shader::setVec3(const char* name, const glm::vec3& v) const
{
    glUniform3fv(glGetUniformLocation(m_id, name), 1, glm::value_ptr(v));
}

void Shader::setMat4(const char* name, const glm::mat4& m) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, glm::value_ptr(m));
}

// ── Компиляция одного шейдера заданного типа ─────────────────────────────────
GLuint Shader::compile(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    // Проверяем успешность компиляции
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        glDeleteShader(s);
        throw std::runtime_error(std::string("Shader compile error:\n") + log);
    }
    return s;
}