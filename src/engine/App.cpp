// Главный класс приложения: окно, камера, ландшафт, редактор вокселей, ImGui.

#include "engine/App.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cmath>

// ── Вспомогательная функция: прочитать текстовый файл (для шейдеров) ─────────
static std::string readFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error(std::string("Cannot open file: ") + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ── Статические колбэки GLFW ─────────────────────────────────────────────────
// GLFW — C-библиотека, требует обычные функции. Методы класса передать
// напрямую нельзя (у метода скрытый аргумент this). Поэтому создаём
// статические «прокладки»:
//   1. В конструкторе сохраняем this через glfwSetWindowUserPointer
//   2. Здесь достаём this через glfwGetWindowUserPointer
//   3. Дёргаем метод класса с правильным this

static void framebufferSizeCallback(GLFWwindow*, int w, int h)
{
    glViewport(0, 0, w, h);   // подстроить область вывода под размер окна
}

static void cursorPosCallback(GLFWwindow* w, double x, double y)
{
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    app->onMouseMove(x, y);
}

static void scrollCallback(GLFWwindow* w, double dx, double dy)
{
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    app->onScroll(dx, dy);
}

static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods)
{
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    app->onMouseButton(button, action, mods);
}

// OpenGL Debug Output — выводим только серьёзные ошибки
static void GLAPIENTRY glDebugCallback(GLenum, GLenum type, GLuint,
                                        GLenum severity, GLsizei,
                                        const GLchar* msg, const void*)
{
    if (severity == GL_DEBUG_SEVERITY_HIGH || type == GL_DEBUG_TYPE_ERROR)
        std::fprintf(stderr, "[GL ERROR] %s\n", msg);
}

// ── Конструктор: пошаговая инициализация всех систем ─────────────────────────
// 1. GLFW (окно + OpenGL-контекст)
// 2. GLAD (загрузка функций OpenGL 4.6)
// 3. ImGui (отладочная панель)
// 4. Шейдер и начальная карта (плоская земля)
// 5. Генерация ландшафта через Marching Cubes
App::App()
{
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");

    // Запрашиваем OpenGL 4.6 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    m_window = glfwCreateWindow(m_width, m_height, "Kopach3D - Marching Cubes", nullptr, nullptr);
    if (!m_window) { glfwTerminate(); throw std::runtime_error("glfwCreateWindow failed"); }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);   // VSync — без разрывов кадра

    if (!gladLoadGL(glfwGetProcAddress))
        throw std::runtime_error("gladLoadGL failed");

    // Настройка отладочного вывода OpenGL (без уведомлений)
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                          GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    // Привязываем колбэки GLFW
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // стартуем с захватом мыши

    // Тест глубины (z-buffer) и отсечение невидимых граней
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Инициализация ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    // Загрузка шейдера ландшафта из файлов
    std::string vertSrc = readFile("assets/shaders/terrain.vert");
    std::string fragSrc = readFile("assets/shaders/terrain.frag");
    m_shader = std::make_unique<Shader>(vertSrc.c_str(), fragSrc.c_str());

    // Создаём мир 64×32×64 и заполняем плоской землёй (y < 8 — твёрдое)
    m_world.init(64, 32, 64);
    for (int x = 0; x < m_world.getSizeX(); ++x) {
        for (int z = 0; z < m_world.getSizeZ(); ++z) {
            for (int y = 0; y < m_world.getSizeY(); ++y) {
                if (y < 8)
                    m_world.setDensity(x, y, z, -1.0f); // твёрдое
                else
                    m_world.setDensity(x, y, z,  1.0f); // воздух
            }
        }
    }

    // Строим меш из вокселей
    buildTerrain();
}

// ── Деструктор: убираем за собой ─────────────────────────────────────────────
App::~App()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

// ── Главный цикл ─────────────────────────────────────────────────────────────
// Каждый кадр: ввод → логика → отрисовка → показ на экране
void App::run()
{
    double prevTime = glfwGetTime();
    while (!glfwWindowShouldClose(m_window))
    {
        double now = glfwGetTime();
        float  dt  = static_cast<float>(now - prevTime);
        prevTime   = now;

        glfwPollEvents();     // события мыши, клавы и окна
        processInput(dt);     // движение камеры, переключение режимов
        update(dt);           // логика мира (пока пусто)
        render();             // 3D-сцена + ImGui
        glfwSwapBuffers(m_window); // показ кадра
    }
}

// ── Движение мыши → поворот камеры ───────────────────────────────────────────
void App::onMouseMove(double x, double y)
{
    if (m_captureMouse)
        m_camera.onMouseMove(x, y);
}

// ── Клик мыши → редактирование блоков ────────────────────────────────────────
// ЛКМ — удалить (сделать воздухом), ПКМ — добавить (сделать твёрдым).
// Режимы кисти:
//   0 — сфера (плавный градиент от центра к краю)
//   1-3 — линия вдоль оси X, Y или Z
//   4-6 — плоскость XZ, XY или YZ
void App::onMouseButton(int button, int action, int /*mods*/)
{
    // Обрабатываем клик только если мышь захвачена и ImGui не перехватил событие
    if (!m_captureMouse) return;
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (action != GLFW_PRESS) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        // Пускаем луч из камеры и ищем первый твёрдый блок
        Camera::Ray ray = m_camera.getRay();
        float step   = 0.1f;
        float maxDist = 20.0f;
        glm::vec3 currentPos = ray.origin;

        for (float d = 0.0f; d < maxDist; d += step)
        {
            currentPos += ray.dir * step;
            int vx = static_cast<int>(std::round(currentPos.x));
            int vy = static_cast<int>(std::round(currentPos.y));
            int vz = static_cast<int>(std::round(currentPos.z));

            if (!m_world.isInside(vx, vy, vz)) continue;

            // Нашли твёрдый блок (плотность < 0)
            if (m_world.getDensity(vx, vy, vz) < 0.0f)
            {
                // Для ПКМ (добавление) — отступаем назад по лучу,
                // чтобы новый блок появился перед поверхностью, а не внутри
                int bx = vx, by = vy, bz = vz;
                if (button == GLFW_MOUSE_BUTTON_RIGHT)
                {
                    glm::vec3 prevPos = currentPos - ray.dir * step * 2.0f;
                    bx = static_cast<int>(std::round(prevPos.x));
                    by = static_cast<int>(std::round(prevPos.y));
                    bz = static_cast<int>(std::round(prevPos.z));
                }

                m_brushPos = glm::vec3(bx, by, bz);

                float targetDensity = (button == GLFW_MOUSE_BUTTON_LEFT) ? 1.0f : -1.0f;
                float radius = static_cast<float>(m_brushRadius);

                // Применяем кисть в зависимости от выбранного режима
                if (m_brushMode == 0) // Сфера с плавным градиентом
                {
                    for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx)
                    for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy)
                    for (int dz = -m_brushRadius; dz <= m_brushRadius; ++dz)
                    {
                        int wx = bx + dx, wy = by + dy, wz = bz + dz;
                        if (!m_world.isInside(wx, wy, wz)) continue;
                        float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy + dz*dz));
                        if (dist <= radius)
                        {
                            float t = glm::clamp(dist / radius, 0.0f, 1.0f);
                            // Линейный градиент: центр = -1, граница = 0
                            // Marching Cubes даст гладкую поверхность на уровне 0
                            float resultDensity;
                            if (targetDensity < 0.0f) {
                                // Добавление: не размягчаем уже твёрдое
                                resultDensity = -1.0f + t;
                                float cur = m_world.getDensity(wx, wy, wz);
                                if (cur < resultDensity) resultDensity = cur;
                            } else {
                                resultDensity = 1.0f; // удаление: чистый воздух
                            }
                            m_world.setDensity(wx, wy, wz, resultDensity);
                        }
                    }
                }
                else if (m_brushMode == 1) // Линия по оси X
                {
                    for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx)
                        m_world.setDensity(bx + dx, by, bz, targetDensity);
                }
                else if (m_brushMode == 2) // Линия по оси Y
                {
                    for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy)
                        m_world.setDensity(bx, by + dy, bz, targetDensity);
                }
                else if (m_brushMode == 3) // Линия по оси Z
                {
                    for (int dz = -m_brushRadius; dz <= m_brushRadius; ++dz)
                        m_world.setDensity(bx, by, bz + dz, targetDensity);
                }
                else if (m_brushMode == 4) // Плоскость XZ (горизонтальная, Y фиксирован)
                {
                    for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx)
                    for (int dz = -m_brushRadius; dz <= m_brushRadius; ++dz)
                        m_world.setDensity(bx + dx, by, bz + dz, targetDensity);
                }
                else if (m_brushMode == 5) // Плоскость XY (вертикальная, Z фиксирован)
                {
                    for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx)
                    for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy)
                        m_world.setDensity(bx + dx, by + dy, bz, targetDensity);
                }
                else if (m_brushMode == 6) // Плоскость YZ (вертикальная, X фиксирован)
                {
                    for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy)
                    for (int dz = -m_brushRadius; dz <= m_brushRadius; ++dz)
                        m_world.setDensity(bx, by + dy, bz + dz, targetDensity);
                }

                // Перестраиваем меш после каждого клика
                buildTerrain();
                break;
            }
        }
    }
}

// ── Скролл колёсиком → изменение FOV камеры ──────────────────────────────────
void App::onScroll(double /*dx*/, double dy)
{
    m_camera.fov -= static_cast<float>(dy) * 5.0f;
    if (m_camera.fov < 10.f)  m_camera.fov = 10.f;
    if (m_camera.fov > 120.f) m_camera.fov = 120.f;
}

// ── Обработка клавиатуры каждый кадр ──────────────────────────────────────────
void App::processInput(float dt)
{
    // ESC — выход
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);

    // TAB — переключить захват мыши (камера ↔ ImGui)
    static bool tabWasPressed = false;
    bool tabIsPressed = glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_PRESS;

    if (tabIsPressed && !tabWasPressed) {
        m_captureMouse = !m_captureMouse;
        if (m_captureMouse) {
            // Режим камеры: курсор скрыт, ImGui не получает ввод
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
            m_camera.resetMouse();
        } else {
            // Режим UI: курсор видим, ImGui получает ввод
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;
        }
    }
    tabWasPressed = tabIsPressed;

    // Движение камеры WASD/QE — только когда мышь захвачена
    if (m_captureMouse) {
        m_camera.update(m_window, dt);
    }

    // F1 — переключить wireframe (каркасный режим)
    static bool f1WasPressed = false;
    if (glfwGetKey(m_window, GLFW_KEY_F1) == GLFW_PRESS) {
        if (!f1WasPressed) {
            m_wireframe = !m_wireframe;
            f1WasPressed = true;
        }
    } else {
        f1WasPressed = false;
    }

    // Постоянный рейкастинг — показываем позицию кисти даже без клика
    if (m_captureMouse) {
        Camera::Ray ray = m_camera.getRay();
        float step = 0.5f;
        float maxDist = 20.0f;
        glm::vec3 currentPos = ray.origin;
        bool found = false;

        for (float d = 0.0f; d < maxDist; d += step)
        {
            currentPos += ray.dir * step;
            int vx = static_cast<int>(std::round(currentPos.x));
            int vy = static_cast<int>(std::round(currentPos.y));
            int vz = static_cast<int>(std::round(currentPos.z));

            if (!m_world.isInside(vx, vy, vz)) continue;
            if (m_world.getDensity(vx, vy, vz) < 0.0f)
            {
                m_brushPos = glm::vec3(vx, vy, vz);
                found = true;
                break;
            }
        }
        if (!found)
            m_brushPos = glm::vec3(-999.f); // прячем кисть, если луч ни во что не попал
    } else {
        m_brushPos = glm::vec3(-999.f);
    }
}

// ── Логика мира (пока пусто, под будущие версии) ─────────────────────────────
void App::update(float /*dt*/)
{
}

// ── Отрисовка одного кадра ────────────────────────────────────────────────────
// Порядок:
//   1. Очистка экрана и z-буфера
//   2. Настройка шейдера (матрицы, свет)
//   3. Рисование всех чанков ландшафта
//   4. Превью кисти (wireframe-сфера или отрезок)
//   5. Отрисовка ImGui поверх 3D-сцены
void App::render()
{
    // Подгоняем viewport под текущий размер окна
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.45f, 0.65f, 0.85f, 1.f); // светло-голубое небо
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Режим отрисовки: сплошные треугольники или каркас
    glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);

    float aspect = m_width > 0 ? float(m_width) / float(m_height) : 1.f;

    // Передаём параметры в шейдер
    m_shader->bind();
    m_shader->setMat4("uView",        m_camera.viewMatrix());
    m_shader->setMat4("uProj",        m_camera.projMatrix(aspect));
    m_shader->setVec3("uLightDir",    m_lightDir);
    m_shader->setVec3("uLightColor",  m_lightColor);
    m_shader->setVec3("uObjectColor", m_terrainColor);
    m_shader->setVec3("uCameraPos",   m_camera.position());

    // Рисуем все чанки
    for (auto& chunk : m_chunks)
        chunk.draw();

    m_shader->unbind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // ── Превью кисти (wireframe) ──────────────────────────────────────────────
    if (m_showBrush && m_captureMouse && m_brushPos.x > -900.f)
    {
        glDisable(GL_CULL_FACE);              // видим всю сферу насквозь
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        m_shader->bind();
        m_shader->setMat4("uView",        m_camera.viewMatrix());
        m_shader->setMat4("uProj",        m_camera.projMatrix(aspect));
        m_shader->setVec3("uLightDir",    m_lightDir);
        m_shader->setVec3("uLightColor",  glm::vec3(0.f)); // без освещения
        m_shader->setVec3("uCameraPos",   m_camera.position());

        // Цвет кисти зависит от зажатой кнопки
        glm::vec3 brushColor(1.0f, 1.0f, 0.0f); // жёлтый — по умолчанию
        if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            brushColor = glm::vec3(1.0f, 0.2f, 0.2f); // красный — удаление
        else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            brushColor = glm::vec3(0.2f, 1.0f, 0.2f); // зелёный — добавление

        m_shader->setVec3("uObjectColor", brushColor);

        float radius = static_cast<float>(m_brushRadius);
        int segments = 16;

        // Рисуем три окружности (XY, XZ, YZ) — получается wireframe-сфера
        for (int plane = 0; plane < 3; ++plane)
        {
            std::vector<glm::vec3> circleVerts;
            for (int i = 0; i <= segments; ++i)
            {
                float angle = 2.0f * 3.14159265f * float(i) / float(segments);
                float cx = std::cos(angle) * radius;
                float cy = std::sin(angle) * radius;
                glm::vec3 pt;
                if (plane == 0)      pt = m_brushPos + glm::vec3(cx, cy, 0.f); // XY
                else if (plane == 1) pt = m_brushPos + glm::vec3(cx, 0.f, cy); // XZ
                else                 pt = m_brushPos + glm::vec3(0.f, cx, cy); // YZ
                circleVerts.push_back(pt);
            }

            // Создаём временный VAO/VBO для одной окружности
            GLuint vao, vbo;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(glm::vec3),
                         circleVerts.data(), GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(circleVerts.size()));
            glBindVertexArray(0);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }

        // Для линейных режимов (1-3) рисуем отрезок вдоль оси вместо сферы
        if (m_brushMode >= 1 && m_brushMode <= 3)
        {
            glm::vec3 axisDir;
            if (m_brushMode == 1)      axisDir = glm::vec3(1.f, 0.f, 0.f);
            else if (m_brushMode == 2) axisDir = glm::vec3(0.f, 1.f, 0.f);
            else                       axisDir = glm::vec3(0.f, 0.f, 1.f);

            std::vector<glm::vec3> lineVerts;
            lineVerts.push_back(m_brushPos - axisDir * radius);
            lineVerts.push_back(m_brushPos + axisDir * radius);

            GLuint vao, vbo;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(glm::vec3),
                         lineVerts.data(), GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glDrawArrays(GL_LINES, 0, 2);
            glBindVertexArray(0);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }

        m_shader->unbind();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
    }

    // ── Отрисовка ImGui ───────────────────────────────────────────────────────
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    renderUI();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ── Панель ImGui — настройки в реальном времени ──────────────────────────────
void App::renderUI()
{
    ImGui::Begin("Kopach3D");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Pos: %.1f %.1f %.1f",
                m_camera.position().x,
                m_camera.position().y,
                m_camera.position().z);
    ImGui::Separator();

    ImGui::Checkbox("Wireframe", &m_wireframe);
    ImGui::Separator();

    ImGui::Text("World");
    if (ImGui::Button("Save Map")) {
        m_world.saveToFile("map.bin");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Map")) {
        if (m_world.loadFromFile("map.bin")) {
            buildTerrain();
        }
    }

    ImGui::Separator();
    ImGui::Text("Brush");
    ImGui::Checkbox("Show brush preview", &m_showBrush);
    ImGui::SliderInt("Radius", &m_brushRadius, 1, 10);

    const char* brushModes[] = { "Sphere (smooth)", "Axis X", "Axis Y", "Axis Z",
                                  "Plane XZ (horiz)", "Plane XY (vert)", "Plane YZ (vert)" };
    ImGui::Combo("Mode", &m_brushMode, brushModes, 7);

    if (m_brushPos.x > -900.f) {
        ImGui::Text("Brush pos: %.0f %.0f %.0f", m_brushPos.x, m_brushPos.y, m_brushPos.z);
    } else {
        ImGui::Text("Brush: no target");
    }

    ImGui::Separator();
    ImGui::ColorEdit3("Terrain color", &m_terrainColor.x);
    ImGui::ColorEdit3("Light color",   &m_lightColor.x);
    ImGui::SliderFloat3("Light dir",   &m_lightDir.x, -1.f, 1.f);

    ImGui::Separator();
    ImGui::Text("TAB  - toggle mouse capture");
    ImGui::Text("WASD/QE - move,  mouse - look");
    ImGui::Text("LMB - remove,  RMB - add");
    ImGui::Text("Scroll - zoom FOV");
    ImGui::End();
}

// ── Генерация ландшафта: разбиваем мир на чанки и запускаем Marching Cubes ───
void App::buildTerrain()
{
    int chunksX = (m_world.getSizeX() + MarchingCubes::SIZE - 1) / MarchingCubes::SIZE;
    int chunksY = (m_world.getSizeY() + MarchingCubes::SIZE - 1) / MarchingCubes::SIZE;
    int chunksZ = (m_world.getSizeZ() + MarchingCubes::SIZE - 1) / MarchingCubes::SIZE;

    m_chunks.clear();
    m_chunks.resize(chunksX * chunksY * chunksZ);

    int idx = 0;
    for (int cx = 0; cx < chunksX; ++cx)
    for (int cy = 0; cy < chunksY; ++cy)
    for (int cz = 0; cz < chunksZ; ++cz)
    {
        // Заполняем поле плотности для этого чанка
        VoxelWorld::DensityField field;
        m_world.fillDensityField(field, cx, cy, cz);

        // Строим треугольники в этом чанке
        std::vector<Vertex> verts;
        glm::vec3 offset = glm::vec3(cx, cy, cz) * float(MarchingCubes::SIZE);
        m_mc.build(field, offset, verts);

        // Загружаем на GPU, если есть что рисовать
        if (!verts.empty()) {
            m_chunks[idx].upload(verts);
        }
        idx++;
    }
}