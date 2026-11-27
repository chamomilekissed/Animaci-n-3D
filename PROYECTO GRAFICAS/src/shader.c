/*
 * shader.c
 * ---------------------------------------------------------------------------
 * Compilación de shaders y envío de uniforms. Sigue el capítulo "Shaders" y
 * la clase Shader del juego Breakout del libro.
 * ---------------------------------------------------------------------------
 */
#include <glad/glad.h>
#include <stdio.h>
#include "shader.h"

/* Revisa si la compilación (o el enlazado) falló e imprime el error.
 * Sin esto, un error de sintaxis en GLSL solo daría una pantalla negra. */
static int revisarErrores(unsigned int objeto, int esPrograma, const char *tipo)
{
    int exito;
    char mensaje[1024];
    if (!esPrograma) {
        glGetShaderiv(objeto, GL_COMPILE_STATUS, &exito);
        if (!exito) {
            glGetShaderInfoLog(objeto, sizeof(mensaje), NULL, mensaje);
            fprintf(stderr, "ERROR al compilar shader (%s):\n%s\n", tipo, mensaje);
        }
    } else {
        glGetProgramiv(objeto, GL_LINK_STATUS, &exito);
        if (!exito) {
            glGetProgramInfoLog(objeto, sizeof(mensaje), NULL, mensaje);
            fprintf(stderr, "ERROR al enlazar programa:\n%s\n", mensaje);
        }
    }
    return exito;
}

unsigned int Shader_Compilar(const char *codigoVertice, const char *codigoFragmento)
{
    /* 1) Crear y compilar el vertex shader (procesa cada vértice). */
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &codigoVertice, NULL);
    glCompileShader(vs);
    revisarErrores(vs, 0, "VERTICE");

    /* 2) Crear y compilar el fragment shader (calcula el color de cada píxel). */
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &codigoFragmento, NULL);
    glCompileShader(fs);
    revisarErrores(fs, 0, "FRAGMENTO");

    /* 3) Unirlos en un "programa" que la GPU puede ejecutar. */
    unsigned int programa = glCreateProgram();
    glAttachShader(programa, vs);
    glAttachShader(programa, fs);
    glLinkProgram(programa);
    int ok = revisarErrores(programa, 1, "PROGRAMA");

    /* 4) Ya enlazados, los shaders sueltos no se necesitan. */
    glDeleteShader(vs);
    glDeleteShader(fs);

    return ok ? programa : 0;
}

void Shader_Usar(unsigned int programa) { glUseProgram(programa); }

/*
 * ALTERNATIVA: aquí llamamos glGetUniformLocation() cada vez que mandamos un
 * uniform. Sería más eficiente buscar la ubicación una sola vez al inicio y
 * guardarla en una variable. Con tan pocos objetos la diferencia no se nota,
 * y así el código queda más corto y fácil de leer (el libro hace lo mismo).
 *
 * ALTERNATIVA 2: Uniform Buffer Objects (UBO, capítulo "Advanced GLSL")
 * sirven para compartir matrices entre varios shaders. Para dos shaders no
 * vale la pena la complejidad extra.
 */
void Shader_SetMat4(unsigned int programa, const char *nombre, const Mat4 *m)
{
    /* GL_FALSE = no transponer, porque ya guardamos la matriz por columnas. */
    glUniformMatrix4fv(glGetUniformLocation(programa, nombre), 1, GL_FALSE, m->m);
}

void Shader_SetVec3(unsigned int programa, const char *nombre, Vec3 v)
{
    glUniform3f(glGetUniformLocation(programa, nombre), v.x, v.y, v.z);
}

void Shader_SetFloat(unsigned int programa, const char *nombre, float v)
{
    glUniform1f(glGetUniformLocation(programa, nombre), v);
}
