#pragma once

// Камера от первого лица — движение WASD, поворот мышью, FOV скроллом.
// Углы Эйлера: Yaw (влево-вправо) и Pitch (вверх-вниз).

#include <glm/glm.hpp>

struct GLFWwindow;

class Camera
{
public:
    // Создать камеру в заданной точке (по умолчанию — центр карты 64x32x64)
    Camera(glm::vec3 position = {32.f, 25.f, 64.f});

    // Передвинуть камеру согласно нажатым клавишам
    void update(GLFWwindow* window, float dt);

    // Повернуть камеру мышью (dx, dy — смещение от прошлого кадра)
    void onMouseMove(double xpos, double ypos);

    // Сбросить накопленное смещение мыши (после переключения TAB, чтобы не было рывка)
    void resetMouse() { m_firstMouse = true; }

    // Матрица вида (lookAt) и матрица проекции (перспектива)
    glm::mat4 viewMatrix() const;
    glm::mat4 projMatrix(float aspect, float nearZ = 0.1f, float farZ = 1000.f) const;

    // Текущая позиция и направление взгляда
    glm::vec3 position() const { return m_pos; }
    glm::vec3 front()    const { return m_front; }

    // Луч из камеры вперёд — для поиска блока под прицелом
    struct Ray { glm::vec3 origin; glm::vec3 dir; };
    Ray getRay() const { return { m_pos, m_front }; }

    float fov         = 60.f;   // угол обзора по вертикали
    float speed       = 20.f;   // скорость движения (ед./сек)
    float sensitivity = 0.1f;   // чувствительность поворота мыши

private:
    // Пересчитать векторы m_front, m_right, m_up по текущим углам
    void updateVectors();

    glm::vec3 m_pos;               // положение в мире
    glm::vec3 m_front = {0.f, 0.f, -1.f}; // куда смотрим
    glm::vec3 m_up    = {0.f, 1.f,  0.f}; // глобальный верх
    glm::vec3 m_right = {1.f, 0.f,  0.f}; // право

    float m_yaw   = -90.f;   // поворот вокруг Y
    float m_pitch =   0.f;   // поворот вокруг локальной X

    double m_lastX = 0.0, m_lastY = 0.0; // прошлая позиция мыши
    bool   m_firstMouse = true;          // флаг первого движения мыши
};
