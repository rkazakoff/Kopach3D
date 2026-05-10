// 3D-поле плотности вокселей: доступ, сохранение/загрузка, заполнение для Marching Cubes.

#include "terrain/VoxelWorld.h"
#include <fstream>
#include <stdexcept>
#include <cstring>

VoxelWorld::VoxelWorld() = default;

// ── Инициализация: задаём размеры и заполняем массив плотности ───────────────
// Все ячейки = 1.0f (воздух). map.bin может потом перезаписать часть из файла.
void VoxelWorld::init(int sizeX, int sizeY, int sizeZ)
{
    // Ограничиваем размер максимально допустимым
    m_sizeX = std::min(sizeX, MAX_SIZE);
    m_sizeY = std::min(sizeY, MAX_SIZE);
    m_sizeZ = std::min(sizeZ, MAX_SIZE);

    size_t totalSize = static_cast<size_t>(m_sizeX) * m_sizeY * m_sizeZ;
    m_data.resize(totalSize, 1.0f);
}

// ── Перевод трёхмерных координат в индекс одномерного массива ────────────────
// X меняется быстрее всего: index = z * sizeY * sizeX + y * sizeX + x
size_t VoxelWorld::getIndex(int x, int y, int z) const
{
    return static_cast<size_t>(z) * m_sizeX * m_sizeY +
           static_cast<size_t>(y) * m_sizeX +
           static_cast<size_t>(x);
}

// ── Прочитать / записать плотность одного вокселя ────────────────────────────
float VoxelWorld::getDensity(int x, int y, int z) const
{
    if (!isInside(x, y, z)) return 1.0f; // за пределами = воздух
    return m_data[getIndex(x, y, z)];
}

void VoxelWorld::setDensity(int x, int y, int z, float density)
{
    if (!isInside(x, y, z)) return;
    m_data[getIndex(x, y, z)] = density;
}

// ── Проверка: лежит ли точка внутри границ мира ──────────────────────────────
bool VoxelWorld::isInside(int x, int y, int z) const
{
    return x >= 0 && x < m_sizeX &&
           y >= 0 && y < m_sizeY &&
           z >= 0 && z < m_sizeZ;
}

// ── Вытащить кусок поля (SIZE+1)³ для одного чанка ──────────────────────────
// chunkX/Y/Z — индекс чанка в сетке чанков (не вокселей).
// Каждый чанк = MarchingCubes::SIZE вокселей.
void VoxelWorld::fillDensityField(DensityField& field,
                                   int chunkX, int chunkY, int chunkZ) const
{
    for (int x = 0; x <= MarchingCubes::SIZE; ++x)
    for (int y = 0; y <= MarchingCubes::SIZE; ++y)
    for (int z = 0; z <= MarchingCubes::SIZE; ++z)
    {
        int worldX = chunkX * MarchingCubes::SIZE + x;
        int worldY = chunkY * MarchingCubes::SIZE + y;
        int worldZ = chunkZ * MarchingCubes::SIZE + z;
        field[x][y][z] = getDensity(worldX, worldY, worldZ);
    }
}

// ── Сохранение карты в бинарный файл ─────────────────────────────────────────
// Формат: три int (размеры), затем sizeX*sizeY*sizeZ float (плотности).
bool VoxelWorld::saveToFile(const std::string& filename) const
{
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // Заголовок: размеры по трём осям
    file.write(reinterpret_cast<const char*>(&m_sizeX), sizeof(m_sizeX));
    file.write(reinterpret_cast<const char*>(&m_sizeY), sizeof(m_sizeY));
    file.write(reinterpret_cast<const char*>(&m_sizeZ), sizeof(m_sizeZ));

    // Все значения плотности
    size_t dataSize = m_data.size() * sizeof(float);
    file.write(reinterpret_cast<const char*>(m_data.data()), dataSize);

    return file.good();
}

// ── Загрузка карты из бинарного файла ────────────────────────────────────────
bool VoxelWorld::loadFromFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // Читаем размеры
    int sx, sy, sz;
    file.read(reinterpret_cast<char*>(&sx), sizeof(sx));
    file.read(reinterpret_cast<char*>(&sy), sizeof(sy));
    file.read(reinterpret_cast<char*>(&sz), sizeof(sz));

    if (!file.good()) return false;

    // Проверяем, что размеры корректны
    if (sx <= 0 || sy <= 0 || sz <= 0 ||
        sx > MAX_SIZE || sy > MAX_SIZE || sz > MAX_SIZE)
        return false;

    // Инициализируем массив подходящего размера
    init(sx, sy, sz);

    // Читаем сами данные
    size_t dataSize = m_data.size() * sizeof(float);
    file.read(reinterpret_cast<char*>(m_data.data()), dataSize);

    return file.good();
}