// Меш на GPU — загрузка вершин, отрисовка, освобождение ресурсов.

#include "engine/Mesh.h"
#include <utility>

// ── Деструктор: освободить OpenGL-хэндлы ─────────────────────────────────────
Mesh::~Mesh()
{
    release();
}

// ── Перемещающий конструктор ─────────────────────────────────────────────────
// Забираем хэндлы у другого меша, его обнуляем.
Mesh::Mesh(Mesh&& o) noexcept
    : m_vao(o.m_vao), m_vbo(o.m_vbo), m_count(o.m_count)
{
    o.m_vao   = 0;
    o.m_vbo   = 0;
    o.m_count = 0;
}

// ── Перемещающее присваивание ────────────────────────────────────────────────
Mesh& Mesh::operator=(Mesh&& o) noexcept
{
    if (this != &o)
    {
        release();             // сначала освобождаем свои ресурсы
        m_vao   = o.m_vao;
        m_vbo   = o.m_vbo;
        m_count = o.m_count;
        o.m_vao   = 0;
        o.m_vbo   = 0;
        o.m_count = 0;
    }
    return *this;
}

// ── Загрузить массив вершин в видеопамять ────────────────────────────────────
// Если VAO/VBO ещё не созданы — создаём.
// layout: location 0 = position (vec3), location 1 = normal (vec3)
void Mesh::upload(const std::vector<Vertex>& vertices)
{
    m_count = static_cast<GLsizei>(vertices.size());

    if (m_vao == 0)
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(),
                 GL_DYNAMIC_DRAW);   // меш может часто обновляться

    // position — атрибут 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));

    // normal — атрибут 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));

    glBindVertexArray(0);
}

// ── Нарисовать меш как набор треугольников ───────────────────────────────────
void Mesh::draw() const
{
    if (m_count == 0) return;
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_count);
    glBindVertexArray(0);
}

// ── Освободить ресурсы GPU ───────────────────────────────────────────────────
void Mesh::release()
{
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao   = 0;
    m_vbo   = 0;
    m_count = 0;
}