/*
 * main.c  —  PacMan 3D en primera persona
 * ---------------------------------------------------------------------------
 * Equivale a program.cpp del juego Breakout de LearnOpenGL:
 *   1. Crea la ventana con GLFW y carga OpenGL 3.3 con GLAD.
 *   2. Registra las funciones que reciben teclado, mouse y cambio de tamaño.
 *   3. Corre el "ciclo de juego": entrada -> actualizar -> dibujar.
 *
 * Controles:
 *   Mouse / flechas izq-der : girar          W A S D / flechas : caminar
 *   C : cambiar 1a / 3a persona              Enter : reiniciar al ganar
 *   Esc : salir
 * ---------------------------------------------------------------------------
 */

/* GLAD SIEMPRE antes que GLFW: GLAD trae los encabezados de OpenGL y GLFW
 * revisa si ya se incluyeron (el libro advierte esto en "Hello Window"). */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "juego.h"

#define ANCHO_INICIAL 1024
#define ALTO_INICIAL  720

/* El juego es global porque las funciones "callback" de GLFW no reciben
 * parámetros nuestros. (Breakout hace lo mismo con su objeto Breakout.)
 * ALTERNATIVA: glfwSetWindowUserPointer() permite guardar un puntero al
 * juego dentro de la ventana y evitar la variable global. Es más "limpio",
 * pero agrega pasos que distraen de lo importante. */
static Juego juego;

/* Para el mouse: recordar la posición anterior (capítulo Camera). */
static float ultimoX = ANCHO_INICIAL / 2.0f, ultimoY = ALTO_INICIAL / 2.0f;
static int   primerMouse = 1;

/* ----------------------------- Callbacks --------------------------------- */

/*
 * Se llama cada vez que se presiona o suelta una tecla.
 * ALTERNATIVA: leer el teclado con glfwGetKey() dentro del ciclo (como hace
 * el libro en los capítulos de cámara). Preferimos el callback, igual que
 * Breakout, porque así detectamos cuándo se SUELTA la tecla y evitamos
 * repeticiones con teclasProcesadas.
 */
static void callbackTeclado(GLFWwindow *ventana, int tecla, int scancode, int accion, int mods)
{
    (void)scancode; (void)mods;   /* no los usamos; esto evita warnings */

    if (tecla == GLFW_KEY_ESCAPE && accion == GLFW_PRESS)
        glfwSetWindowShouldClose(ventana, 1);

    if (tecla >= 0 && tecla < 1024) {
        if (accion == GLFW_PRESS) {
            juego.teclas[tecla] = 1;
        } else if (accion == GLFW_RELEASE) {
            juego.teclas[tecla] = 0;
            juego.teclasProcesadas[tecla] = 0;  /* ya se puede volver a usar */
        }
    }
}

/* Se llama cada vez que se mueve el mouse (capítulo Camera, "Mouse input"). */
static void callbackMouse(GLFWwindow *ventana, double xpos, double ypos)
{
    (void)ventana;
    if (primerMouse) {          /* evita un salto brusco la primera vez */
        ultimoX = (float)xpos;
        ultimoY = (float)ypos;
        primerMouse = 0;
    }
    float dx = (float)xpos - ultimoX;
    float dy = ultimoY - (float)ypos;   /* invertido: en pantalla y crece hacia abajo */
    ultimoX = (float)xpos;
    ultimoY = (float)ypos;

    Camara_ProcesarMouse(&juego.camara, dx, dy);
}

/* Se llama si cambia el tamaño de la ventana: ajustamos el viewport y la
 * relación de aspecto para que la imagen no se vea estirada. */
static void callbackTamano(GLFWwindow *ventana, int ancho, int alto)
{
    (void)ventana;
    glViewport(0, 0, ancho, alto);
    juego.ancho = ancho;
    juego.alto  = alto;
}

/* Pone el marcador en el título de la ventana.
 * ALTERNATIVA: dibujar texto en pantalla con FreeType (capítulo "Text
 * Rendering", lo usa Breakout para las vidas). Es lo más bonito, pero
 * requiere otra biblioteca y un shader más; el título basta para el marcador. */
static void actualizarTitulo(GLFWwindow *ventana)
{
    char titulo[160];
    if (juego.estado == JUEGO_GANADO)
        snprintf(titulo, sizeof(titulo),
                 "PacMan 3D  |  GANASTE! Puntos: %d  |  Enter = jugar otra vez",
                 juego.puntos);
    else
        snprintf(titulo, sizeof(titulo),
                 "PacMan 3D  |  Puntos: %d  |  Bolitas: %d  |  Camara: %s (C)",
                 juego.puntos, juego.bolitasRestantes,
                 juego.camara.modo == CAM_PRIMERA_PERSONA ? "1a persona" : "3a persona");
    glfwSetWindowTitle(ventana, titulo);
    juego.tituloSucio = 0;
}

/* -------------------------------- main ----------------------------------- */

int main(void)
{
    /* 1) Iniciar GLFW y pedir OpenGL 3.3 "core" (sin funciones antiguas). */
    if (!glfwInit()) {
        fprintf(stderr, "No se pudo iniciar GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  /* necesario en macOS */
#endif

    /* 2) Crear la ventana y hacer su contexto el "actual". */
    GLFWwindow *ventana = glfwCreateWindow(ANCHO_INICIAL, ALTO_INICIAL, "PacMan 3D", NULL, NULL);
    if (!ventana) {
        fprintf(stderr, "No se pudo crear la ventana (¿tu tarjeta soporta OpenGL 3.3?)\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(ventana);

    /* 3) Cargar las funciones de OpenGL con GLAD.
     * ALTERNATIVA: GLEW hace lo mismo (glewInit()). El libro usa GLAD y es
     * más ligero porque solo genera las funciones de la versión elegida. */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "No se pudo iniciar GLAD\n");
        return -1;
    }

    /* 4) Registrar callbacks y capturar el mouse (se oculta y no se sale
     * de la ventana, como en un juego de disparos en primera persona). */
    glfwSetKeyCallback(ventana, callbackTeclado);
    glfwSetCursorPosCallback(ventana, callbackMouse);
    glfwSetFramebufferSizeCallback(ventana, callbackTamano);
    glfwSetInputMode(ventana, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /* 5) Configuración de OpenGL.
     * En pantallas de alta densidad (Retina) el framebuffer mide más que la
     * ventana, así que pedimos su tamaño real. */
    int anchoFB, altoFB;
    glfwGetFramebufferSize(ventana, &anchoFB, &altoFB);
    glViewport(0, 0, anchoFB, altoFB);
    glEnable(GL_DEPTH_TEST);      /* lo cercano tapa lo lejano */
    glEnable(GL_BLEND);           /* transparencia para las mandíbulas */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* 6) Iniciar el juego (shaders, mallas y nivel). */
    Juego_Iniciar(&juego, anchoFB, altoFB);

    /* 7) Ciclo principal. deltaTime = tiempo que tardó el cuadro anterior. */
    float ultimoCuadro = 0.0f;
    while (!glfwWindowShouldClose(ventana)) {
        float ahora = (float)glfwGetTime();
        float dt = ahora - ultimoCuadro;
        ultimoCuadro = ahora;
        /* Si la ventana se congeló (por ejemplo al arrastrarla), dt puede
         * ser enorme y PacMan "atravesaría" paredes. Lo limitamos. */
        if (dt > 0.05f) dt = 0.05f;

        glfwPollEvents();                      /* leer teclado y mouse */

        Juego_ProcesarEntrada(&juego, dt);     /* mover a PacMan       */
        Juego_Actualizar(&juego, dt);          /* comer, puntos, ganar */

        glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Juego_Dibujar(&juego);                 /* dibujar la escena    */

        if (juego.tituloSucio) actualizarTitulo(ventana);

        /* Doble búfer: dibujamos en el de atrás y luego lo intercambiamos,
         * así nunca se ve la imagen a medio dibujar. */
        glfwSwapBuffers(ventana);
    }

    /* 8) Liberar todo antes de salir. */
    Juego_Liberar(&juego);
    glfwTerminate();
    return 0;
}
