// Вершинный шейдер ландшафта.
// На вход: позиция вершины + нормаль (из Marching Cubes).
// На выход: позиция во фрагментном шейдере + нормаль для освещения.
// Умножает вершину на матрицы uView и uProj для проецирования на экран.

#version 460 core

layout(location = 0) in vec3 aPosition;   // позиция вершины в мире
layout(location = 1) in vec3 aNormal;      // нормаль (уже нормализована)

uniform mat4 uView;   // матрица вида (камера)
uniform mat4 uProj;   // матрица перспективной проекции

out vec3 vNormal;     // нормаль → фрагментный шейдер
out vec3 vFragPos;    // позиция → фрагментный шейдер (для specular)

void main()
{
    vFragPos    = aPosition;
    vNormal     = aNormal;
    gl_Position = uProj * uView * vec4(aPosition, 1.0);
}