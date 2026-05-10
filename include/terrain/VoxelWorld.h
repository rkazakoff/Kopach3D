#pragma once

// Трёхмерное поле плотности вокселей.
// Положительное значение = воздух, отрицательное = твёрдый блок.
// Хранится в одномерном массиве (X меняется быстрее всего).

#include "terrain/MarchingCubes.h"
#include <vector>
#include <array>
#include <string>

class VoxelWorld
{
public:
    static constexpr int MAX_SIZE = 64;   // максимальный размер мира по любой оси

    VoxelWorld();

    // Задать размеры мира (не больше MAX_SIZE) и заполнить воздухом
    void init(int sizeX, int sizeY, int sizeZ);

    // Прочитать / записать плотность одного вокселя
    float getDensity(int x, int y, int z) const;
    void  setDensity(int x, int y, int z, float density);

    // Лежит ли точка внутри границ мира
    bool isInside(int x, int y, int z) const;

    // Размеры мира
    int getSizeX() const { return m_sizeX; }
    int getSizeY() const { return m_sizeY; }
    int getSizeZ() const { return m_sizeZ; }

    // Сохранить / загрузить карту в бинарный файл
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    // Заполнить DensityField для одного чанка Marching Cubes
    using DensityField = std::array<std::array<std::array<float, MarchingCubes::SIZE+1>,
                                                            MarchingCubes::SIZE+1>,
                                                            MarchingCubes::SIZE+1>;
    void fillDensityField(DensityField& field, int chunkX, int chunkY, int chunkZ) const;

private:
    int m_sizeX = 0, m_sizeY = 0, m_sizeZ = 0;

    // Все воксели в одном плоском массиве: index = z*sizeX*sizeY + y*sizeX + x
    std::vector<float> m_data;

    size_t getIndex(int x, int y, int z) const;
};
