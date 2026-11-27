/*
 * juego.c
 * ---------------------------------------------------------------------------
 * Lógica y dibujo del juego. Es la parte equivalente a game.cpp del juego
 * Breakout del libro, adaptada a 3D y a un laberinto tipo PacMan.
 * ---------------------------------------------------------------------------
 */
#include <glad/glad.h>
#include <GLFW/glfw3.h>    /* solo para los códigos de tecla GLFW_KEY_... */
#include <string.h>
#include <math.h>
#include "juego.h"
#include "shader.h"
#include "shaders_fuente.h"

/* ----------------------------- Constantes -------------------------------- */

#define ALTURA_OJOS       0.35f   /* altura de la cámara en 1a persona      */
#define RADIO_BOLITA      0.07f
#define RADIO_BOLITA_GDE  0.16f
#define DURACION_MORDIDA  0.30f   /* segundos que tarda un "chomp"          */
#define VEL_GIRO_TECLADO  120.0f  /* grados/seg al girar con las flechas    */

static const Vec3 COLOR_FONDO = { 0.01f, 0.01f, 0.03f };

/*
 * El nivel. Cada carácter es una casilla de 1x1 en el mundo:
 * la casilla (fila f, columna c) tiene su centro en x = c, z = f.
 *
 * ALTERNATIVA: el libro carga los niveles de Breakout desde archivos de
 * texto (GameLevel::Load). Con un solo nivel lo dejamos aquí para no
 * depender de rutas de archivos; para varios niveles convendría leerlos
 * de archivo con fopen()/fgets().
 */
static const char *NIVEL_1[] = {
    "###############",
    "#o.....#.....o#",
    "#.###..#..###.#",
    "#.#.........#.#",
    "#.#.##.#.##.#.#",
    "#......#......#",
    "###.#.....#.###",
    "#...#..P..#...#",
    "#.###.###.###.#",
    "#.............#",
    "#.#.###.###.#.#",
    "#.#....#....#.#",
    "#.###..#..###.#",
    "#o.....#.....o#",
    "###############",
};
#define FILAS_NIVEL_1 ((int)(sizeof(NIVEL_1) / sizeof(NIVEL_1[0])))

/* =========================================================================
 *                           INICIO Y CARGA DE NIVEL
 * ========================================================================= */

void Juego_Iniciar(Juego *j, int ancho, int alto)
{
    memset(j, 0, sizeof(*j));      /* todo en cero, incluidas las teclas */
    j->ancho = ancho;
    j->alto  = alto;

    /* 1) Compilar los dos programas de shaders. */
    j->shaderEscena = Shader_Compilar(VERTICE_ESCENA, FRAGMENTO_ESCENA);
    j->shaderBoca   = Shader_Compilar(VERTICE_BOCA,   FRAGMENTO_BOCA);

    /* 2) Crear la geometría una sola vez. Todas las paredes reutilizan el
     *    mismo cubo y todas las bolitas la misma esfera; solo cambia la
     *    matriz "modelo" con la que se dibujan. */
    j->cubo   = Malla_CrearCubo();
    j->esfera = Malla_CrearEsfera(24, 16);
    j->quad   = Malla_CrearQuadPantalla();

    /* 3) Leer el mapa y colocar a PacMan y las bolitas. */
    Juego_CargarNivel(j);
}

void Juego_CargarNivel(Juego *j)
{
    j->filas = FILAS_NIVEL_1;
    j->cols  = (int)strlen(NIVEL_1[0]);
    j->numBolitas = 0;
    j->puntos = 0;
    j->tiempo = 0.0f;
    j->temporizadorMordida = 0.0f;
    j->estado = JUEGO_ACTIVO;

    /* Valores iniciales del jugador */
    j->jugador.radio      = 0.25f;  /* menor a 0.5 para caber en pasillos  */
    j->jugador.velocidad  = 2.5f;   /* mismo valor que la cámara del libro */
    j->jugador.moviendose = 0;

    /* Recorremos el mapa casilla por casilla. */
    for (int f = 0; f < j->filas; f++) {
        strncpy(j->mapa[f], NIVEL_1[f], MAX_COLS);
        j->mapa[f][MAX_COLS] = '\0';
        for (int c = 0; c < j->cols; c++) {
            char ch = j->mapa[f][c];
            if (ch == '.' || ch == 'o') {
                Bolita *b = &j->bolitas[j->numBolitas++];
                b->x = (float)c;
                b->z = (float)f;
                b->estado = BOLITA_VIVA;
                b->grande = (ch == 'o');
                b->t = 0.0f;
            } else if (ch == 'P') {
                j->jugador.x = (float)c;
                j->jugador.z = (float)f;
            }
        }
    }
    j->bolitasRestantes = j->numBolitas;

    /* yaw = -90° hace que la cámara mire hacia -Z, o sea "hacia arriba"
     * del mapa (igual que el valor inicial del libro). Conservamos el modo
     * de cámara si el jugador ya lo había cambiado. */
    ModoCamara modo = j->camara.modo;
    Camara_Iniciar(&j->camara, -90.0f);
    j->camara.modo = modo;

    j->tituloSucio = 1;
}

/* =========================================================================
 *                               COLISIONES
 * ========================================================================= */

static int esPared(const Juego *j, int f, int c)
{
    if (f < 0 || c < 0 || f >= j->filas || c >= j->cols) return 1; /* fuera = pared */
    return j->mapa[f][c] == '#';
}

/*
 * ¿Un círculo con centro (x,z) y radio r toca alguna pared?
 *
 * Es la prueba "círculo contra AABB" del capítulo "Collision detection" del
 * juego Breakout (la que usa la pelota contra los ladrillos):
 *   1. Buscamos el punto de la caja más cercano al centro del círculo,
 *      "limitando" (clamp) el centro a los bordes de la caja.
 *   2. Si ese punto está a menos de r del centro, hay choque.
 *
 * Solo revisamos las 9 casillas alrededor del jugador en lugar de todas
 * las paredes del mapa: el jugador no puede tocar una pared más lejana.
 */
static int chocaConPared(const Juego *j, float x, float z, float r)
{
    int fc = (int)floorf(z + 0.5f);   /* casilla en la que está el centro */
    int cc = (int)floorf(x + 0.5f);
    for (int f = fc - 1; f <= fc + 1; f++)
        for (int c = cc - 1; c <= cc + 1; c++) {
            if (!esPared(j, f, c)) continue;
            /* La pared (f,c) es una caja de c-0.5..c+0.5 por f-0.5..f+0.5 */
            float px = limitar(x, c - 0.5f, c + 0.5f);
            float pz = limitar(z, f - 0.5f, f + 0.5f);
            float dx = x - px, dz = z - pz;
            /* Comparamos distancias al cuadrado para no sacar raíz. */
            if (dx * dx + dz * dz < r * r) return 1;
        }
    return 0;
}

/*
 * Mueve al jugador, pero primero en X y luego en Z por separado.
 * Si moverse en X choca, se cancela solo X y se conserva Z. Así PacMan
 * "resbala" a lo largo de la pared en vez de quedarse pegado al chocar en
 * diagonal.
 *
 * ALTERNATIVA 1 (la del libro con la pelota): moverse y, si hay choque,
 * empujar el objeto hacia afuera según la dirección de penetración. Es más
 * precisa, pero más larga y con riesgo de "temblores" en las esquinas.
 * ALTERNATIVA 2 (la del PacMan original): moverse de casilla en casilla
 * sobre una cuadrícula. Es muy simple, pero en primera persona con mouse se
 * siente rígido; el movimiento libre se siente más natural.
 */
static void moverConColision(Juego *j, float dx, float dz)
{
    Jugador *p = &j->jugador;
    if (!chocaConPared(j, p->x + dx, p->z, p->radio)) p->x += dx;
    if (!chocaConPared(j, p->x, p->z + dz, p->radio)) p->z += dz;
}

/* =========================================================================
 *                              ENTRADA (TECLADO)
 * ========================================================================= */

void Juego_ProcesarEntrada(Juego *j, float dt)
{
    /* Tecla C: cambiar de cámara. Usamos teclasProcesadas igual que el
     * libro, para que al dejarla presionada no cambie 60 veces por segundo. */
    if (j->teclas[GLFW_KEY_C] && !j->teclasProcesadas[GLFW_KEY_C]) {
        j->camara.modo = (j->camara.modo == CAM_PRIMERA_PERSONA)
                         ? CAM_TERCERA_PERSONA : CAM_PRIMERA_PERSONA;
        j->teclasProcesadas[GLFW_KEY_C] = 1;
        j->tituloSucio = 1;
    }

    if (j->estado == JUEGO_GANADO) {
        /* Enter para volver a jugar (Breakout también usa Enter). */
        if (j->teclas[GLFW_KEY_ENTER] && !j->teclasProcesadas[GLFW_KEY_ENTER]) {
            j->teclasProcesadas[GLFW_KEY_ENTER] = 1;
            Juego_CargarNivel(j);
        }
        j->jugador.moviendose = 0;
        return;
    }

    /* Flechas izquierda/derecha: girar sin mouse. */
    if (j->teclas[GLFW_KEY_LEFT])  j->camara.yaw -= VEL_GIRO_TECLADO * dt;
    if (j->teclas[GLFW_KEY_RIGHT]) j->camara.yaw += VEL_GIRO_TECLADO * dt;
    Camara_ActualizarVectores(&j->camara);

    /* Dirección de caminata relativa a hacia dónde mira PacMan.
     * Usamos el frente "plano" (sin pitch): es el ejercicio de la "cámara
     * FPS verdadera" del capítulo Camera, para no flotar ni hundirse. */
    Vec3 frente  = Camara_FrentePlano(&j->camara);
    Vec3 derecha = v3_normalizar(v3_cruz(frente, j->camara.arribaMundo));
    Vec3 dir = v3(0.0f, 0.0f, 0.0f);

    if (j->teclas[GLFW_KEY_W] || j->teclas[GLFW_KEY_UP])   dir = v3_suma(dir, frente);
    if (j->teclas[GLFW_KEY_S] || j->teclas[GLFW_KEY_DOWN]) dir = v3_resta(dir, frente);
    if (j->teclas[GLFW_KEY_D]) dir = v3_suma(dir, derecha);
    if (j->teclas[GLFW_KEY_A]) dir = v3_resta(dir, derecha);

    j->jugador.moviendose = v3_longitud(dir) > 0.0f;
    if (j->jugador.moviendose) {
        /* Normalizamos para que en diagonal (W + D) no vaya más rápido.
         * Multiplicar por dt (deltaTime) hace que la velocidad sea la misma
         * en una computadora lenta o rápida (capítulo Camera, "Movement speed"). */
        dir = v3_normalizar(dir);
        float paso = j->jugador.velocidad * dt;
        moverConColision(j, dir.x * paso, dir.z * paso);
    }
}

/* =========================================================================
 *                         ACTUALIZAR (LÓGICA DEL JUEGO)
 * ========================================================================= */

void Juego_Actualizar(Juego *j, float dt)
{
    j->tiempo += dt;

    for (int i = 0; i < j->numBolitas; i++) {
        Bolita *b = &j->bolitas[i];

        if (b->estado == BOLITA_VIVA && j->estado == JUEGO_ACTIVO) {
            /* ¿PacMan está encima de la bolita? Prueba "círculo contra
             * punto": la distancia entre centros es menor que el radio.
             * ALTERNATIVA: revisar si están en la misma casilla del mapa
             * (round(x) == b->x). Es más barato, pero la bolita
             * desaparecería de golpe apenas entras a la casilla, antes de
             * llegar a ella, y en primera persona se nota. */
            float dx = j->jugador.x - b->x, dz = j->jugador.z - b->z;
            if (dx * dx + dz * dz < j->jugador.radio * j->jugador.radio) {
                b->estado = BOLITA_COMIENDOSE;   /* empieza la animación */
                b->t = 0.0f;
                j->temporizadorMordida = DURACION_MORDIDA;
            }
        } else if (b->estado == BOLITA_COMIENDOSE) {
            b->t += dt;
            if (b->t >= DURACION_MORDIDA) {      /* terminó la mordida */
                b->estado = BOLITA_COMIDA;
                j->puntos += b->grande ? 50 : 10;  /* puntos como el original */
                j->bolitasRestantes--;
                j->tituloSucio = 1;
            }
        }
    }

    /* --- Animación de la boca (el "waka waka") ---
     * Al caminar, la boca se abre y cierra un poco con una onda seno.
     * Al morder, se cierra por completo: sin(pi * progreso) va 0 -> 1 -> 0. */
    float abreCierra = j->jugador.moviendose
        ? 0.18f + 0.25f * (0.5f + 0.5f * sinf(j->tiempo * 14.0f))
        : 0.18f;   /* en reposo se asoma un poquito el labio */
    float mordida = 0.0f;
    if (j->temporizadorMordida > 0.0f) {
        j->temporizadorMordida -= dt;
        float progreso = 1.0f - j->temporizadorMordida / DURACION_MORDIDA;
        mordida = sinf(PI_F * limitar(progreso, 0.0f, 1.0f));
    }
    j->cierreBoca = fmaxf(abreCierra, mordida);

    /* ¿Ya no quedan bolitas? ¡Ganaste! (en Breakout: IsCompleted()) */
    if (j->estado == JUEGO_ACTIVO && j->bolitasRestantes == 0) {
        j->estado = JUEGO_GANADO;
        j->tituloSucio = 1;
    }
}

/* =========================================================================
 *                                 DIBUJO
 * ========================================================================= */

/* Atajo: dibuja una malla con una matriz modelo y un color. */
static void dibujarObjeto(const Juego *j, const Malla *m, Mat4 modelo,
                          Vec3 color, float emision, float boca)
{
    Shader_SetMat4 (j->shaderEscena, "modelo",   &modelo);
    Shader_SetVec3 (j->shaderEscena, "uColor",   color);
    Shader_SetFloat(j->shaderEscena, "uEmision", emision);
    Shader_SetFloat(j->shaderEscena, "uBoca",    boca);
    Malla_Dibujar(m);
}

void Juego_Dibujar(Juego *j)
{
    Vec3 posJugador = v3(j->jugador.x, 0.0f, j->jugador.z);

    /* --- 1) Matrices de cámara (vista) y de proyección --- */
    Mat4 vista = Camara_Seguir(&j->camara, posJugador, ALTURA_OJOS);
    float aspecto = (j->alto > 0) ? (float)j->ancho / (float)j->alto : 1.0f;
    /* 0.05 de plano cercano: las bolitas que se acercan a la boca se ven
     * hasta muy cerca sin que se "corten". */
    Mat4 proy = m4_perspectiva(radianes(70.0f), aspecto, 0.05f, 100.0f);

    Shader_Usar(j->shaderEscena);
    Shader_SetMat4(j->shaderEscena, "vista", &vista);
    Shader_SetMat4(j->shaderEscena, "proyeccion", &proy);
    Shader_SetVec3(j->shaderEscena, "uVistaPos", j->camara.posicion);
    /* La luz va encima de PacMan, como si llevara una lámpara. */
    Shader_SetVec3(j->shaderEscena, "uLuzPos", v3(j->jugador.x, 0.9f, j->jugador.z));
    Shader_SetVec3(j->shaderEscena, "uColorNiebla", COLOR_FONDO);

    /* --- 2) Piso: un cubo muy aplanado debajo de todo el laberinto --- */
    {
        Vec3 centro = v3((j->cols - 1) * 0.5f, -0.01f, (j->filas - 1) * 0.5f);
        Mat4 modelo = m4_multiplicar(m4_trasladar(centro),
                                     m4_escalar(v3((float)j->cols, 0.02f, (float)j->filas)));
        /* El piso usa escala NO uniforme, pero su normal es (0,1,0) y
         * escalar en Y no cambia su dirección, así que mat3(modelo) sigue
         * funcionando después de normalizar. */
        dibujarObjeto(j, &j->cubo, modelo, v3(0.06f, 0.06f, 0.14f), 0.0f, 0.0f);
    }

    /* --- 3) Paredes: un cubo de 1x1x1 en cada '#' ---
     * ALTERNATIVA: "instancing" (capítulo "Instancing", glDrawArraysInstanced)
     * dibujaría todas las paredes en UNA sola llamada. Con ~100 paredes el
     * ciclo normal va sobrado de rápido y es más fácil de seguir. */
    for (int f = 0; f < j->filas; f++)
        for (int c = 0; c < j->cols; c++)
            if (j->mapa[f][c] == '#') {
                Mat4 modelo = m4_trasladar(v3((float)c, 0.5f, (float)f));
                dibujarObjeto(j, &j->cubo, modelo, v3(0.10f, 0.20f, 0.95f), 0.05f, 0.0f);
            }

    /* --- 4) Bolitas --- */
    /* Punto "boca": a donde vuela la bolita cuando la comes. En primera
     * persona está justo enfrente de la cámara; en tercera, en la boca de
     * la esfera de PacMan. */
    Vec3 puntoBoca = (j->camara.modo == CAM_PRIMERA_PERSONA)
        ? v3_suma(v3_suma(j->camara.posicion, v3_escala(j->camara.frente, 0.12f)),
                  v3(0.0f, -0.04f, 0.0f))
        : v3_suma(v3(j->jugador.x, ALTURA_OJOS, j->jugador.z),
                  v3_escala(Camara_FrentePlano(&j->camara), j->jugador.radio * 0.5f));

    for (int i = 0; i < j->numBolitas; i++) {
        const Bolita *b = &j->bolitas[i];
        if (b->estado == BOLITA_COMIDA) continue;

        float radio = b->grande ? RADIO_BOLITA_GDE : RADIO_BOLITA;
        /* Flotan un poco arriba y abajo; el desfase (x+z) evita que todas
         * suban al mismo tiempo. */
        float y = ALTURA_OJOS + 0.04f * sinf(j->tiempo * 3.0f + b->x + b->z);
        Vec3 pos = v3(b->x, y, b->z);
        /* Las grandes parpadean, como en el original. */
        float brillo = b->grande ? 0.6f + 0.4f * sinf(j->tiempo * 6.0f) : 0.6f;

        if (b->estado == BOLITA_COMIENDOSE) {
            /* En la primera mitad de la mordida la bolita vuela hacia la
             * boca y se encoge; en la segunda mitad ya está "adentro". */
            float p = b->t / (DURACION_MORDIDA * 0.5f);
            if (p >= 1.0f) continue;
            pos = v3_mezclar(pos, puntoBoca, p);
            radio *= (1.0f - 0.7f * p);
        }
        Mat4 modelo = m4_multiplicar(m4_trasladar(pos), m4_escalar(v3(radio, radio, radio)));
        dibujarObjeto(j, &j->esfera, modelo, v3(1.0f, 0.80f, 0.70f), brillo, 0.0f);
    }

    /* --- 5) PacMan (solo en tercera persona; en primera estamos dentro) --- */
    if (j->camara.modo == CAM_TERCERA_PERSONA) {
        float r = j->jugador.radio;
        /* Orden: escalar -> girar hacia donde mira -> llevar a su posición.
         * Usamos -yaw porque rotarY gira en sentido contrario al yaw. */
        Mat4 modelo = m4_multiplicar(m4_trasladar(v3(j->jugador.x, ALTURA_OJOS, j->jugador.z)),
                      m4_multiplicar(m4_rotarY(-radianes(j->camara.yaw)),
                                     m4_escalar(v3(r, r, r))));
        /* Apertura de la boca en radianes: abierta ~0.8, cerrada ~0. */
        float boca = 0.8f * (1.0f - j->cierreBoca) + 0.001f;
        dibujarObjeto(j, &j->esfera, modelo, v3(1.0f, 0.85f, 0.0f), 0.1f, boca);
    }

    /* --- 6) Mandíbulas en primera persona (superposición 2D) ---
     * Se dibujan al final, encima de todo: apagamos la prueba de
     * profundidad para que nada las tape y usamos blending para que sean
     * un poquito transparentes. */
    if (j->camara.modo == CAM_PRIMERA_PERSONA) {
        glDisable(GL_DEPTH_TEST);
        Shader_Usar(j->shaderBoca);
        Shader_SetFloat(j->shaderBoca, "uCierre", j->cierreBoca);
        Malla_Dibujar(&j->quad);
        glEnable(GL_DEPTH_TEST);
    }
}

void Juego_Liberar(Juego *j)
{
    Malla_Liberar(&j->cubo);
    Malla_Liberar(&j->esfera);
    Malla_Liberar(&j->quad);
    glDeleteProgram(j->shaderEscena);
    glDeleteProgram(j->shaderBoca);
}
