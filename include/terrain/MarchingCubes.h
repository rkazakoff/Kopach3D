#pragma once

// Marching Cubes — строит полигональную сетку по 3D-полю плотности.
// Проходит по кубикам (вокселям) и по таблице определяет, какие треугольники
// нужно построить в каждом кубе, чтобы получить поверхность на уровне isoLevel.

#include "engine/Mesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <array>

// Таблицы Marching Cubes (Paul Bourke, public domain)
// edgeTable — какие рёбра куба пересекает поверхность (битовая маска)
// triTable  — индексы рёбер, образующих треугольники (по 3, завершается -1)
namespace mc_tables
{
    extern const int edgeTable[256];
    extern const int triTable[256][16];
}

class MarchingCubes
{
public:
    static constexpr int SIZE = 32;     // размер чанка в вокселях
    float isoLevel = 0.0f;              // порог изоповерхности (0 = граница воздух/твёрдое)

    // Построить треугольники для одного чанка.
    // density — массив (SIZE+1)³ значений плотности в узлах сетки.
    // offset  — смещение чанка в мировых координатах.
    // outVertices — сюда складываются готовые вершины.
    void build(const std::array<std::array<std::array<float, SIZE+1>, SIZE+1>, SIZE+1>& density,
               glm::vec3 offset,
               std::vector<Vertex>& outVertices);

private:
    // Линейная интерполяция точки на ребре между p1 и p2,
    // где плотность переходит через iso
    glm::vec3 interpolate(float iso,
                          glm::vec3 p1, float v1,
                          glm::vec3 p2, float v2) const;
};
