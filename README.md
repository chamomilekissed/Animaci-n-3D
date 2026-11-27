[README.md](https://github.com/user-attachments/files/35514598/README.md)
# PacMan 3D en primera persona (C + OpenGL 3.3)

Un PacMan sin fantasmas: recorres un laberinto 3D y te comes todas las bolitas.
La cámara va en **primera persona**: ves cómo las bolitas vuelan hacia tu boca y
cómo las mandíbulas amarillas se cierran en cada mordida. Con la tecla **C**
cambias a **tercera persona** para ver a PacMan por fuera, con su boca animada.

Está basado en el juego **Breakout** del último capítulo de LearnOpenGL
("In Practice → 2D Game") y en los capítulos *Camera*, *Basic Lighting*,
*Light casters* y *Collision detection*.

## Controles

| Tecla | Acción |
|---|---|
| Mouse / ← → | Girar |
| W A S D / ↑ ↓ | Caminar |
| C | Cambiar entre 1ª y 3ª persona |
| Enter | Jugar otra vez (después de ganar) |
| Esc | Salir |

El marcador (puntos y bolitas restantes) aparece en el título de la ventana.
Bolita chica = 10 puntos, bolita grande = 50 (igual que el original).

## Archivos y su equivalente en el libro

| Archivo | Qué hace | En Breakout (C++) |
|---|---|---|
| `src/main.c` | Ventana, callbacks y ciclo del juego | `program.cpp` |
| `src/juego.c/.h` | Estados, entrada, lógica, colisiones y dibujo | `Game` + `GameLevel` |
| `src/camara.c/.h` | Cámara con yaw/pitch, 1ª y 3ª persona | `Camera` (capítulo Camera) |
| `src/shader.c/.h` | Compilar shaders y mandar uniforms | `Shader` |
| `src/shaders_fuente.h` | Código GLSL (Phong + mandíbulas) | archivos `.vs` / `.fs` |
| `src/malla.c/.h` | Cubo, esfera y cuadrado en la GPU (VAO/VBO/EBO) | `SpriteRenderer` |
| `src/matematicas.h` | Vectores y matrices (reemplaza a GLM) | GLM |

Como C no tiene clases, cada "clase" del libro es un `struct` más funciones
con prefijo (`Juego_Actualizar(&juego, dt)` en lugar de `Breakout.Update(dt)`).

## Cómo compilar

### 1. Generar GLAD (igual que en "Creating a window" del libro)
1. Entra a <https://glad.dav1d.de>.
2. Language: **C/C++** · API gl: **Version 3.3** · Profile: **Core** · marca **Generate a loader**.
3. Descarga el zip y copia su contenido así:
   ```
   externo/glad/include/glad/glad.h
   externo/glad/include/KHR/khrplatform.h
   externo/glad/src/glad.c
   ```

### 2a. Con CMake (Windows, Linux o macOS)
GLFW se descarga solo la primera vez (necesitas internet y `git`).
```
cmake -S . -B build
cmake --build build
./build/pacman3d        (en Windows: build\Debug\pacman3d.exe)
```
En Linux instala antes las dependencias de GLFW:
`sudo apt install build-essential cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`
(y `libwayland-dev libxkbcommon-dev` si usas Wayland).

### 2b. Sin CMake (gcc directo)
- **Linux** (con `libglfw3-dev` instalado):
  `gcc -std=c99 src/*.c externo/glad/src/glad.c -Iexterno/glad/include -lglfw -lGL -lm -ldl -o pacman3d`
- **Windows con MinGW** (con GLFW precompilado para MinGW):
  `gcc -std=c99 src/*.c externo/glad/src/glad.c -Iexterno/glad/include -Iruta/glfw/include -Lruta/glfw/lib-mingw-w64 -lglfw3 -lgdi32 -lopengl32 -o pacman3d.exe`

### 2c. Visual Studio con el proyecto que armaron siguiendo el libro
Si ya tienen un proyecto con GLFW y GLAD configurados (como en "Creating a
window"), basta con agregar los archivos de `src/` al proyecto. Visual Studio
compila los `.c` como C automáticamente.

## Cómo funciona (resumen)

1. **Mapa de texto → mundo 3D.** Cada carácter del mapa es una casilla de 1×1.
   La casilla (fila f, columna c) tiene su centro en `x = c, z = f`.
   `#` = cubo azul, `.` = bolita, `o` = bolita grande, `P` = inicio.
2. **Primera persona.** La cámara se pone en la posición de PacMan a la altura
   de los ojos y mira según yaw/pitch (fórmula de Euler del libro). Para caminar
   se usa el frente "plano" (sin pitch), así no flotas al mirar arriba.
3. **Colisiones.** PacMan es un círculo y cada pared una caja (AABB). Se usa la
   misma prueba círculo-contra-AABB de la pelota de Breakout. El movimiento se
   aplica por eje (primero X, luego Z) para resbalar por las paredes.
4. **Comer.** Si la distancia a una bolita es menor al radio de PacMan, la bolita
   pasa a "comiéndose": vuela hacia la boca y se encoge mientras las
   mandíbulas se cierran (`sin(π·progreso)`), y al terminar suma puntos.
5. **Mandíbulas.** En primera persona se dibuja encima de la escena un cuadrado
   de pantalla completa; el fragment shader pinta dos curvas amarillas (arriba
   y abajo) que se cierran según `uCierre`.
6. **PacMan en tercera persona.** Es una esfera; el fragment shader descarta
   (`discard`) los píxeles dentro de una cuña para formar la boca, y pinta de
   rojo oscuro el interior usando `gl_FrontFacing`.

## Qué otras funciones/técnicas se pudieron usar y por qué elegimos estas

| Lo que usamos | Alternativa | Por qué preferimos lo nuestro |
|---|---|---|
| Matemáticas propias (`matematicas.h`) | **cglm** (GLM para C) | GLM es C++. cglm es excelente, pero es otra dependencia; escribir `lookAt` y `perspective` a mano deja claro qué hace cada matriz. |
| **GLAD** | **GLEW** (`glewInit()`) | Es lo que usa el libro y solo genera la versión que pedimos (3.3 core). |
| Callback de teclado + `teclasProcesadas` | `glfwGetKey()` en el ciclo | El callback detecta cuándo se *suelta* la tecla, así la C no cambia la cámara 60 veces por segundo. Es el mismo truco de Breakout. |
| Variable global `juego` | `glfwSetWindowUserPointer()` | Más sencillo de leer; Breakout hace lo mismo. |
| Shaders en cadenas dentro del código | Archivos `.vs/.fs` con un ResourceManager | Evita el error de "no encuentra el shader" según la carpeta desde donde se ejecuta. A cambio, hay que recompilar al cambiar un shader. |
| `glGetUniformLocation` en cada envío | Guardar las ubicaciones al inicio, o UBOs | Con tan pocos objetos no se nota la diferencia y el código queda más corto. |
| `mat3(modelo)` para las normales | `transpose(inverse(modelo))` | Solo usamos escalas uniformes; la inversa sería trabajo extra en cada vértice. |
| Esfera UV generada a mano | Icoesfera, modelo `.obj` con Assimp, `glutSolidSphere` | La UV sphere es la más fácil de entender. `glutSolidSphere`/`gluSphere` no existen en el perfil core 3.3. |
| Un `glDrawArrays` por pared | **Instancing** (`glDrawArraysInstanced`) | Con ~100 paredes sobra rendimiento; instancing sería la mejora para mapas grandes. |
| Movimiento libre + colisión por eje | Empujar según la penetración (como la pelota de Breakout) o moverse de casilla en casilla (PacMan original) | Por eje es corto y no "tiembla" en esquinas; la cuadrícula se siente rígida en primera persona con mouse. |
| Comer por distancia (círculo) | Comer al entrar a la casilla | Por casilla la bolita desaparecería antes de llegar a ella, y en primera persona se nota. |
| Ángulos de Euler (yaw/pitch) | Cuaterniones | Sin "roll" no hay gimbal lock que evitar, y Euler es la forma en que el libro lo explica. |
| Marcador en el título de la ventana | Texto con **FreeType** (capítulo Text Rendering) | FreeType necesita otra biblioteca y otro shader; el título basta para mostrar los puntos. |
| Niebla en el fragment shader | Sin niebla / `glFog` | `glFog` es del pipeline fijo y no existe en core. La niebla da sensación de profundidad en el pasillo. |

## Ideas para ampliar el proyecto
- Fantasmas que persiguen a PacMan con búsqueda en anchura (BFS) sobre el mapa.
- Texturas en paredes y piso con `stb_image` (capítulo Textures).
- Sonido "waka waka" con irrKlang (como Breakout en "Audio").
- Partículas al comer una bolita grande (capítulo "Particles" de Breakout).
- Varios niveles leídos de archivos de texto (como `GameLevel::Load`).
