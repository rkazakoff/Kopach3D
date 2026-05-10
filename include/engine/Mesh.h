#pragma once

// Меш на GPU — хранит VAO и VBO, умеет загружать вершины и рисовать их.
// Некопируемый (только перемещаемый), потому что владеет OpenGL-хэндлами.

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

// Одна вершина: позиция в пространстве + нормаль для освещения
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

class Mesh
{
public:
    Mesh() = default;
    ~Mesh();

    // Некопируемый — OpenGL-хэндлы нельзя просто скопировать
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Перемещаемый — передаём владение хэндлами
    Mesh(Mesh&& o) noexcept;
    Mesh& operator=(Mesh&& o) noexcept;

    // Загрузить массив вершин в видеобуфер (VAO + VBO)
    void upload(const std::vector<Vertex>& vertices);

    // Нарисовать GL_TRIANGLES
    void draw() const;

    // Пустой ли меш (нет треугольников)
    bool empty() const { return m_count == 0; }

private:
    // Освободить ресурсы GPU
    void release();

    GLuint m_vao   = 0;   // Vertex Array Object
    GLuint m_vbo   = 0;   // Vertex Buffer Object
    GLsizei m_count = 0;   // количество вершин
};
