# CMakeLists.txt — PacMan 3D (C + OpenGL 3.3)
# ---------------------------------------------------------------------------
# CMake genera el proyecto para tu compilador (Visual Studio, MinGW, Make...).
# GLFW se descarga solo la primera vez (FetchContent). GLAD lo generas tú en
# https://glad.dav1d.de y lo copias a externo/glad (ver README.md).
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.14)
project(PacMan3D C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- GLFW: ventana, teclado y mouse --------------------------------------
include(FetchContent)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

# --- El juego -------------------------------------------------------------
add_executable(pacman3d
    src/main.c
    src/juego.c
    src/camara.c
    src/shader.c
    src/malla.c
    externo/glad/src/glad.c)

target_include_directories(pacman3d PRIVATE externo/glad/include)
target_link_libraries(pacman3d PRIVATE glfw OpenGL::GL)

# En Linux hay que enlazar la biblioteca matemática (sinf, cosf...) y dl.
if(UNIX AND NOT APPLE)
    target_link_libraries(pacman3d PRIVATE m ${CMAKE_DL_LIBS})
endif()
