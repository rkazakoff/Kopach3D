#include "engine/Camera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

// ── Конструктор ───────────────────────────────────────────────────────────────
// Yaw = -90° и Pitch = 0° — камера смотрит вдоль -Z (вглубь сцены).
Camera::Camera(glm::vec3 position)
    : m_pos(position)
{
    m_yaw   = -90.0f;
    m_pitch =  0.0f;
    updateVectors();
}

// ── Движение камеры в локальных осях ─────────────────────────────────────────
// m_front — вперёд/назад (W/S)
// m_right — влево/вправо (A/D)
// m_up    — вверх/вниз (E/Q)
// Скорость умножается на dt, чтобы не зависеть от FPS.
void Camera::update(GLFWwindow* window, float dt)
{
    float vel = speed * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_pos += m_front * vel;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_pos -= m_front * vel;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_pos -= m_right * vel;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_pos += m_right * vel;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_pos += m_up    * vel;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) m_pos -= m_up    * vel;
}

// ── Поворот камеры мышью ─────────────────────────────────────────────────────
// dx — горизонтальное движение мыши → меняет Yaw (влево-вправо)
// dy — вертикальное движение мыши   → меняет Pitch (вверх-вниз)
// Инвертируем dy, потому что экранный Y растёт вниз.
// Pitch ограничен ±89°, чтобы камера не перевернулась «через голову».
void Camera::onMouseMove(double xpos, double ypos)
{
    if (m_firstMouse)
    {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float dx = static_cast<float>(xpos - m_lastX) * sensitivity;
    float dy = static_cast<float>(m_lastY - ypos) * sensitivity;

    m_lastX = xpos;
    m_lastY = ypos;

    m_yaw   += dx;
    m_pitch += dy;

    if (m_pitch > 89.0f)  m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    updateVectors();
}

// ── Матрица вида (lookAt) ────────────────────────────────────────────────────
// Переводит мировые координаты в систему координат камеры.
glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_pos, m_pos + m_front, m_up);
}

// ── Матрица перспективной проекции ───────────────────────────────────────────
// fov — угол обзора по вертикали, aspect — ширина/высота окна,
// nearZ/farZ — ближняя и дальняя плоскости отсечения.
glm::mat4 Camera::projMatrix(float aspect, float nearZ, float farZ) const
{
    return glm::perspective(glm::radians(fov), aspect, nearZ, farZ);
}

// ── Пересчёт векторов камеры по углам Yaw и Pitch ────────────────────────────
// Используем сферическую систему координат:
//   front.x = cos(yaw) * cos(pitch)
//   front.y = sin(pitch)
//   front.z = sin(yaw) * cos(pitch)
// right = front × worldUp (worldUp = (0,1,0))
// up    = right × front
void Camera::updateVectors()
{
    glm::vec3 f;
    f.x = cos(glm::radians(m_yaw))   * cos(glm::radians(m_pitch));
    f.y = sin(glm::radians(m_pitch));
    f.z = sin(glm::radians(m_yaw))   * cos(glm::radians(m_pitch));

    m_front = glm::normalize(f);
    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}