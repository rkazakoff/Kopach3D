// Фрагментный (пиксельный) шейдер ландшафта.
// Модель освещения Blinn-Phong: ambient + diffuse + specular.
// Источник — направленный свет (как солнце).

#version 460 core

in vec3 vNormal;       // нормаль из вершинного шейдера
in vec3 vFragPos;      // позиция фрагмента в мире

uniform vec3 uLightDir;     // направление К источнику света (нормализовано)
uniform vec3 uLightColor;   // цвет света
uniform vec3 uObjectColor;  // цвет материала
uniform vec3 uCameraPos;    // позиция камеры (для specular)

out vec4 FragColor;    // итоговый цвет пикселя

void main()
{
    vec3 N = normalize(vNormal);    // нормализуем — интерполяция могла исказить длину
    vec3 L = normalize(-uLightDir); // вектор ОТ поверхности к источнику

    // Ambient — равномерная подсветка, чтобы тени не были абсолютно чёрными
    float ambientStr = 0.15;
    vec3 ambient = ambientStr * uLightColor;

    // Diffuse — чем больше угол между N и L, тем темнее (Lambert)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular — блик (Blinn-Phong: используем halfway-вектор H)
    vec3 V = normalize(uCameraPos - vFragPos);  // вектор к камере
    vec3 H = normalize(L + V);                  // halfway между L и V
    float spec = pow(max(dot(N, H), 0.0), 32.0); // 32 = блеск (чем больше, тем резче блик)
    vec3 specular = 0.2 * spec * uLightColor;

    vec3 result = (ambient + diffuse + specular) * uObjectColor;
    FragColor = vec4(result, 1.0);
}