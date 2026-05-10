#pragma once

// Обёртка над OpenGL-шейдерной программой.
// Компилирует вершинный и фрагментный шейдеры из исходников,
// связывает их в программу и позволяет передавать uniform-переменные.

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
    // Создать программу из исходников GLSL (вершинный + фрагментный)
    Shader(const char* vertSrc, const char* fragSrc);
    ~Shader();

    // Включить / выключить эту шейдерную программу
    void bind()   const;
    void unbind() const;

    // Передать значения uniform-переменных в шейдер
    void setInt  (const char* name, int v)              const;
    void setFloat(const char* name, float v)            const;
    void setVec3 (const char* name, const glm::vec3& v) const;
    void setMat4 (const char* name, const glm::mat4& m) const;

    // ID программы (OpenGL-хэндл)
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;

    // Скомпилировать один шейдер заданного типа
    static GLuint compile(GLenum type, const char* src);
};
