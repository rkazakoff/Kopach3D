#pragma once

// Главный класс приложения — окно, камера, рендер, редактор вокселей.
// Связывает все части движка в единый цикл.

#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/Mesh.h"
#include "terrain/VoxelWorld.h"
#include "terrain/MarchingCubes.h"

#include <memory>
#include <vector>

class App
{
public:
    App();
    ~App();

    // Запуск главного цикла (работает пока окно открыто)
    void run();

    // Колбэки ввода — вызываются из статических функций GLFW
    void onMouseMove(double x, double y);       // движение мыши → поворот камеры
    void onScroll(double dx, double dy);        // колёсико → зум (FOV)
    void onMouseButton(int button, int action, int mods); // клик → редактирование блоков

private:
    void processInput(float dt);   // клавиатура каждый кадр
    void update(float dt);         // логика мира
    void render();                 // отрисовка 3D-сцены и ImGui
    void renderUI();               // панель ImGui с настройками
    void buildTerrain();           // перестроить меш из вокселей (Marching Cubes)

    // Редактирование вокселей
    void handleVoxelEditing();
    bool raycastVoxel(const Camera::Ray& ray, glm::ivec3& hitPos, glm::ivec3& hitNormal);
    void setVoxel(const glm::ivec3& pos, float density);

    // Сохранение / загрузка карты в бинарный файл
    void saveMap(const std::string& filename);
    void loadMap(const std::string& filename);

    // Окно GLFW
    struct GLFWwindow* m_window = nullptr;
    int m_width  = 1280;
    int m_height = 720;

    // Основные объекты движка
    Camera                        m_camera;      // камера от первого лица
    std::unique_ptr<Shader>       m_shader;      // шейдер освещения
    std::vector<Mesh>             m_chunks;      // чанки ландшафта (меши на GPU)
    VoxelWorld                    m_world;       // 3D-поле плотности вокселей
    MarchingCubes                 m_mc;          // генератор полигональной сетки

    // Режимы
    bool m_captureMouse = true;   // мышь захвачена камерой (TAB переключает)
    bool m_wireframe    = false;  // отрисовка треугольников (F1 переключает)

    // Параметры кисти редактирования
    int   m_brushRadius = 2;      // радиус кисти в вокселях
    int   m_brushMode   = 0;      // форма: 0=сфера, 1-3=оси XYZ, 4-6=плоскости
    glm::vec3 m_brushPos = {0.f, 0.f, 0.f}; // текущая позиция кисти
    bool  m_showBrush   = true;   // показывать превью кисти

    // Параметры освещения (Phong)
    glm::vec3 m_lightDir    = glm::normalize(glm::vec3(0.4f, -1.f, 0.3f));
    glm::vec3 m_lightColor  = {1.f, 0.95f, 0.8f};
    glm::vec3 m_terrainColor = {0.35f, 0.55f, 0.25f};
};
