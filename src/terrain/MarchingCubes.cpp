// Marching Cubes — генерация треугольников по 3D-полю плотности.
// Алгоритм: для каждого куба (вокселя) проверяем 8 его вершин,
// по таблице определяем, какие треугольники строить внутри куба.
// Вершины треугольников интерполируются по рёбрам куба.

#include "terrain/MarchingCubes.h"
#include <glm/glm.hpp>
#include <cmath>
using namespace mc_tables;

/*
 * Marching Cubes — алгоритм извлечения изоповерхности из скалярного поля.
 *
 * Идея:
 *   Пространство разбито на кубы (воксели). В каждой вершине куба
 *   хранится значение плотности. Если в вершине плотность < isoLevel,
 *   вершина считается "внутри" поверхности, иначе "снаружи".
 *
 *   По комбинации 8 вершин (256 вариантов) таблица triTable говорит,
 *   какие треугольники нужно построить. Вершины треугольников —
 *   точки на рёбрах куба, интерполированные между "внутренним"
 *   и "внешним" значением.
 */

// 8 вершин куба в локальных координатах (0..1)
static constexpr glm::vec3 kCornerOffset[8] = {
    {0,0,0},{1,0,0},{1,1,0},{0,1,0},
    {0,0,1},{1,0,1},{1,1,1},{0,1,1}
};

// Пары индексов вершин, образующие 12 рёбер куба
static constexpr int kEdgePairs[12][2] = {
    {0,1},{1,2},{2,3},{3,0},   // нижняя грань (y=0)
    {4,5},{5,6},{6,7},{7,4},   // верхняя грань (y=1)
    {0,4},{1,5},{2,6},{3,7}    // вертикальные рёбра
};

// ── Основной метод: построить треугольники для одного чанка ──────────────────
// density — (SIZE+1)³ значений плотности в узлах сетки.
// offset  — мировое смещение этого чанка (чтобы стыковать с соседними).
// outVertices — выходной массив вершин (дополняется, не очищается извне).
void MarchingCubes::build(
    const std::array<std::array<std::array<float, SIZE+1>, SIZE+1>, SIZE+1>& density,
    glm::vec3 offset,
    std::vector<Vertex>& outVertices)
{
    outVertices.clear();

    // Проходим по всем кубам сетки SIZE×SIZE×SIZE
    for (int x = 0; x < SIZE; ++x)
    for (int y = 0; y < SIZE; ++y)
    for (int z = 0; z < SIZE; ++z)
    {
        // Читаем плотность в 8 вершинах текущего куба
        float val[8];
        for (int i = 0; i < 8; ++i)
        {
            int cx = x + static_cast<int>(kCornerOffset[i].x);
            int cy = y + static_cast<int>(kCornerOffset[i].y);
            int cz = z + static_cast<int>(kCornerOffset[i].z);
            val[i] = density[cx][cy][cz];
        }

        // Битовая маска: бит i = 1 если вершина i "внутри" (плотность < isoLevel)
        // Это индекс строки в таблицах edgeTable и triTable.
        int cubeIdx = 0;
        for (int i = 0; i < 8; ++i)
            if (val[i] < isoLevel)
                cubeIdx |= (1 << i);

        // edgeTable[cubeIdx] — битовая маска рёбер, пересекаемых поверхностью.
        // Если 0 — поверхность не проходит через этот куб, пропускаем.
        if (edgeTable[cubeIdx] == 0) continue;

        // Интерполяция точек пересечения на всех 12 рёбрах куба,
        // где плотность переходит через isoLevel.
        glm::vec3 edgeVerts[12];
        for (int e = 0; e < 12; ++e)
        {
            if (edgeTable[cubeIdx] & (1 << e))
            {
                int a = kEdgePairs[e][0];
                int b = kEdgePairs[e][1];
                glm::vec3 pa = glm::vec3(x,y,z) + kCornerOffset[a];
                glm::vec3 pb = glm::vec3(x,y,z) + kCornerOffset[b];
                edgeVerts[e] = interpolate(isoLevel, pa, val[a], pb, val[b]);
            }
        }

        // По таблице triTable строим треугольники.
        // triTable[cubeIdx] — список индексов рёбер, по три на треугольник.
        // Завершается -1.
        //
        // ВАЖНО: порядок вершин p0,p2,p1 (а не p0,p1,p2) даёт правильный
        // winding (CCW) для лицевой стороны. Без этой инверсии треугольники
        // смотрели бы нормалями ВНИЗ и отсекались бы GL_CULL_FACE при виде сверху.
        for (int t = 0; triTable[cubeIdx][t] != -1; t += 3)
        {
            glm::vec3 p0 = edgeVerts[triTable[cubeIdx][t  ]] + offset;
            glm::vec3 p1 = edgeVerts[triTable[cubeIdx][t+1]] + offset;
            glm::vec3 p2 = edgeVerts[triTable[cubeIdx][t+2]] + offset;

            // Нормаль через векторное произведение двух рёбер треугольника
            glm::vec3 n = glm::normalize(glm::cross(p2 - p0, p1 - p0));

            outVertices.push_back({p0, n});
            outVertices.push_back({p2, n});
            outVertices.push_back({p1, n});
        }
    }
}

/*
 * Линейная интерполяция точки на ребре между p1 и p2,
 * где плотность переходит через iso.
 *
 *   t = (iso - v1) / (v2 - v1)
 *   результат = p1 + t * (p2 - p1)
 *
 * Если v1 ≈ v2 (ребро почти параллельно изоповерхности),
 * возвращаем середину ребра чтобы избежать деления на ноль.
 */
glm::vec3 MarchingCubes::interpolate(float iso,
                                      glm::vec3 p1, float v1,
                                      glm::vec3 p2, float v2) const
{
    if (std::abs(v2 - v1) < 1e-6f) return (p1 + p2) * 0.5f;
    float t = (iso - v1) / (v2 - v1);
    return p1 + t * (p2 - p1);
}