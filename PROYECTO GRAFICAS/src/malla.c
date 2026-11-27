/*
 * malla.c
 * ---------------------------------------------------------------------------
 * Crea en la GPU las figuras que usa el juego: cubo, esfera y un cuadrado
 * que cubre la pantalla.
 * ---------------------------------------------------------------------------
 */
#include <glad/glad.h>
#include <stdlib.h>
#include "malla.h"
#include "matematicas.h"

/* Configura un VAO con vértices de 6 floats: posición (x,y,z) + normal. */
static void configurarAtributosPosNormal(void)
{
    /* Atributo 0 = posición: 3 floats, cada vértice mide 6 floats,
     * y la posición empieza en el byte 0. */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    /* Atributo 1 = normal: empieza después de los 3 floats de posición. */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

/* ------------------------------------------------------------------------
 * CUBO de lado 1 centrado en el origen (de -0.5 a 0.5).
 * Son los mismos 36 vértices con normales del capítulo "Basic Lighting".
 * Cada cara tiene su propia normal, por eso se repiten las esquinas
 * (una esquina pertenece a 3 caras con normales distintas).
 * ---------------------------------------------------------------------- */
Malla Malla_CrearCubo(void)
{
    static const float v[] = {
        /* posición             normal      */
        -0.5f,-0.5f,-0.5f,  0, 0,-1,   0.5f,-0.5f,-0.5f,  0, 0,-1,   0.5f, 0.5f,-0.5f,  0, 0,-1,
         0.5f, 0.5f,-0.5f,  0, 0,-1,  -0.5f, 0.5f,-0.5f,  0, 0,-1,  -0.5f,-0.5f,-0.5f,  0, 0,-1,

        -0.5f,-0.5f, 0.5f,  0, 0, 1,   0.5f,-0.5f, 0.5f,  0, 0, 1,   0.5f, 0.5f, 0.5f,  0, 0, 1,
         0.5f, 0.5f, 0.5f,  0, 0, 1,  -0.5f, 0.5f, 0.5f,  0, 0, 1,  -0.5f,-0.5f, 0.5f,  0, 0, 1,

        -0.5f, 0.5f, 0.5f, -1, 0, 0,  -0.5f, 0.5f,-0.5f, -1, 0, 0,  -0.5f,-0.5f,-0.5f, -1, 0, 0,
        -0.5f,-0.5f,-0.5f, -1, 0, 0,  -0.5f,-0.5f, 0.5f, -1, 0, 0,  -0.5f, 0.5f, 0.5f, -1, 0, 0,

         0.5f, 0.5f, 0.5f,  1, 0, 0,   0.5f, 0.5f,-0.5f,  1, 0, 0,   0.5f,-0.5f,-0.5f,  1, 0, 0,
         0.5f,-0.5f,-0.5f,  1, 0, 0,   0.5f,-0.5f, 0.5f,  1, 0, 0,   0.5f, 0.5f, 0.5f,  1, 0, 0,

        -0.5f,-0.5f,-0.5f,  0,-1, 0,   0.5f,-0.5f,-0.5f,  0,-1, 0,   0.5f,-0.5f, 0.5f,  0,-1, 0,
         0.5f,-0.5f, 0.5f,  0,-1, 0,  -0.5f,-0.5f, 0.5f,  0,-1, 0,  -0.5f,-0.5f,-0.5f,  0,-1, 0,

        -0.5f, 0.5f,-0.5f,  0, 1, 0,   0.5f, 0.5f,-0.5f,  0, 1, 0,   0.5f, 0.5f, 0.5f,  0, 1, 0,
         0.5f, 0.5f, 0.5f,  0, 1, 0,  -0.5f, 0.5f, 0.5f,  0, 1, 0,  -0.5f, 0.5f,-0.5f,  0, 1, 0,
    };

    Malla m = { 0 };
    m.numVertices = 36;

    glGenVertexArrays(1, &m.vao);   /* 1) crear el VAO y activarlo         */
    glGenBuffers(1, &m.vbo);        /* 2) crear el VBO                     */
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    /* 3) copiar los vértices a la GPU. GL_STATIC_DRAW = no van a cambiar. */
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    configurarAtributosPosNormal(); /* 4) explicar el formato del arreglo  */
    glBindVertexArray(0);           /* 5) desactivar para no modificarlo   */
    return m;
}

/* ------------------------------------------------------------------------
 * ESFERA de radio 1 (tipo "UV sphere", como un globo terráqueo):
 *   - "pilas"    = rebanadas horizontales (latitud)
 *   - "sectores" = gajos verticales (longitud)
 * Como la esfera es de radio 1 y está centrada en el origen, la normal de
 * cada vértice es igual a su posición. ¡Muy conveniente!
 *
 * ALTERNATIVA: una "icoesfera" (subdividir un icosaedro) tiene triángulos
 * más parejos, o cargar un modelo .obj con Assimp (capítulo "Model
 * Loading"). La UV sphere es la más fácil de entender y calcular a mano.
 *
 * ALTERNATIVA 2: en OpenGL antiguo existía gluSphere()/glutSolidSphere(),
 * pero son del pipeline fijo y NO funcionan en el perfil "core" 3.3 que
 * usa el libro.
 * ---------------------------------------------------------------------- */
Malla Malla_CrearEsfera(int sectores, int pilas)
{
    int numVert = (pilas + 1) * (sectores + 1);
    float *v = (float *)malloc(sizeof(float) * 6 * numVert);
    unsigned int *ind = (unsigned int *)malloc(sizeof(unsigned int) * 6 * pilas * sectores);
    int k = 0;

    /* Vértices: recorremos latitud (phi) y longitud (theta). */
    for (int i = 0; i <= pilas; i++) {
        float phi = PI_F / 2.0f - PI_F * (float)i / pilas;   /* de +90° a -90° */
        float y = sinf(phi);
        float r = cosf(phi);                                /* radio del anillo */
        for (int j = 0; j <= sectores; j++) {
            float theta = 2.0f * PI_F * (float)j / sectores; /* de 0° a 360° */
            float x = r * cosf(theta);
            float z = r * sinf(theta);
            v[k++] = x; v[k++] = y; v[k++] = z;   /* posición */
            v[k++] = x; v[k++] = y; v[k++] = z;   /* normal = posición */
        }
    }

    /* Índices: cada "cuadrito" entre dos anillos se parte en 2 triángulos.
     * Usar índices (EBO) evita repetir vértices: cada uno se comparte con
     * hasta 6 triángulos (capítulo "Hello Triangle", Element Buffer Objects).
     * El orden de cada triángulo es antihorario visto desde afuera, que es lo
     * que OpenGL considera "cara de enfrente" (capítulo "Face culling"). Lo
     * necesitamos para pintar distinto el interior de la boca de PacMan. */
    int n = 0;
    for (int i = 0; i < pilas; i++) {
        int a = i * (sectores + 1);      /* inicio del anillo de arriba */
        int b = a + sectores + 1;        /* inicio del anillo de abajo  */
        for (int j = 0; j < sectores; j++, a++, b++) {
            if (i != 0) {                /* el polo norte solo necesita 1 triángulo */
                ind[n++] = a; ind[n++] = a + 1; ind[n++] = b;
            }
            if (i != pilas - 1) {        /* el polo sur también */
                ind[n++] = a + 1; ind[n++] = b + 1; ind[n++] = b;
            }
        }
    }

    Malla m = { 0 };
    m.numIndices = n;
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * numVert, v, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);   /* el VAO recuerda el EBO */
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * n, ind, GL_STATIC_DRAW);
    configurarAtributosPosNormal();
    glBindVertexArray(0);

    /* La GPU ya tiene su copia; liberamos la memoria del CPU. */
    free(v);
    free(ind);
    return m;
}

/* ------------------------------------------------------------------------
 * Cuadrado que cubre toda la pantalla (de -1 a 1 en x e y), hecho de dos
 * triángulos. Solo tiene posición 2D. Es igual al quad que usa Breakout
 * para el post-procesamiento.
 * ---------------------------------------------------------------------- */
Malla Malla_CrearQuadPantalla(void)
{
    static const float v[] = {
        -1.0f, -1.0f,   1.0f, -1.0f,   1.0f,  1.0f,
        -1.0f, -1.0f,   1.0f,  1.0f,  -1.0f,  1.0f,
    };
    Malla m = { 0 };
    m.numVertices = 6;
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return m;
}

void Malla_Dibujar(const Malla *m)
{
    glBindVertexArray(m->vao);
    if (m->numIndices > 0)
        glDrawElements(GL_TRIANGLES, m->numIndices, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(GL_TRIANGLES, 0, m->numVertices);
    glBindVertexArray(0);
}

void Malla_Liberar(Malla *m)
{
    glDeleteVertexArrays(1, &m->vao);
    glDeleteBuffers(1, &m->vbo);
    if (m->ebo) glDeleteBuffers(1, &m->ebo);
    m->vao = m->vbo = m->ebo = 0;
}
