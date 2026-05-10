# ── Минимальная версия CMake ──────────────────────────────────────────────────
cmake_minimum_required(VERSION 3.20)

# ── Проект: название + язык C++ ───────────────────────────────────────────────
project(Kopach3D VERSION 0.1.0 LANGUAGES CXX)

# ── Стандарт C++20 (нужен для constexpr, std::span и т.д.) ────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Выходные директории (exe → bin/, lib → lib/) ──────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── Автоматическая загрузка внешних библиотек через Git (FetchContent) ────────
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# GLFW — создание окна, обработка ввода, OpenGL-контекст (zlib license)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
# Отключаем всё лишнее у GLFW — нам нужна только сама библиотека
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# GLM — математическая библиотека: векторы, матрицы, quaternion (MIT license)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

# Dear ImGui — графический интерфейс для панели настроек (MIT license)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6
    GIT_SHALLOW    TRUE
)

# Скачиваем и подключаем все зависимости одновременно
FetchContent_MakeAvailable(glfw glm imgui)

# ── GLAD — загрузчик функций OpenGL (лежит в проекте, не скачивается, MIT) ────
add_library(glad STATIC
    libs/glad/src/gl.c
)
target_include_directories(glad PUBLIC libs/glad/include)

# ── Сборка ImGui как статической библиотеки ───────────────────────────────────
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw)

# ── Исходники движка — собираем все .cpp из папки src/ ────────────────────────
file(GLOB_RECURSE ENGINE_SOURCES
    src/*.cpp
)

# Главная цель — исполняемый файл Kopach3D.exe
add_executable(Kopach3D ${ENGINE_SOURCES})

# Подключаем свои заголовки
target_include_directories(Kopach3D PRIVATE
    include
    src
)

# Линкуем все зависимости в один exe
target_link_libraries(Kopach3D PRIVATE
    glad
    glfw
    glm::glm
    imgui_lib
)

# ── Windows: скрываем консоль — приложение становится оконным, а не консольным ─
if(MSVC)
    target_link_options(Kopach3D PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()

# ── Платформо-зависимые линковки ──────────────────────────────────────────────
if(WIN32)
    target_link_libraries(Kopach3D PRIVATE opengl32)
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(Kopach3D PRIVATE OpenGL::GL dl)
endif()

# ── Предупреждения компилятора ────────────────────────────────────────────────
if(MSVC)
    target_compile_options(Kopach3D PRIVATE /W4)
else()
    target_compile_options(Kopach3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Копируем папку assets (шейдеры) в папку с exe после каждой сборки ─────────
add_custom_command(TARGET Kopach3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Kopach3D>/assets
    COMMENT "Copying assets to output directory"
)

# Сообщение об успешной настройке (видно в логе CMake)
message(STATUS "Kopach3D configured. Build dir: ${CMAKE_BINARY_DIR}")
