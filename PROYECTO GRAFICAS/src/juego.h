/*
 * juego.h
 * ---------------------------------------------------------------------------
 * Equivalente en C de la clase "Game" del juego Breakout del libro.
 * El libro organiza el juego en 4 pasos que se repiten cada cuadro:
 *     ProcessInput -> Update -> Render
 * más Init al inicio. Aquí son:
 *     Juego_Iniciar, Juego_ProcesarEntrada, Juego_Actualizar, Juego_Dibujar
 * Como C no tiene clases, todos los datos van en el struct "Juego" y cada
 * función recibe un puntero a él (es lo que en C++ sería "this").
 * ---------------------------------------------------------------------------
 */
#ifndef JUEGO_H
#define JUEGO_H

#include "camara.h"
#include "malla.h"

#define MAX_FILAS   32
#define MAX_COLS    32
#define MAX_BOLITAS (MAX_FILAS * MAX_COLS)

/* Estados del juego (Breakout tiene GAME_ACTIVE, GAME_MENU y GAME_WIN).
 * No hay fantasmas, así que no hay estado de "perdiste". */
typedef enum { JUEGO_ACTIVO, JUEGO_GANADO } EstadoJuego;

/* Cada bolita pasa por 3 estados: viva -> comiéndose (animación) -> comida */
typedef enum { BOLITA_COMIDA, BOLITA_VIVA, BOLITA_COMIENDOSE } EstadoBolita;

typedef struct {
    float x, z;          /* posición en el piso (la altura y es fija)     */
    float radio;         /* PacMan se trata como un círculo para chocar   */
    float velocidad;     /* unidades (casillas) por segundo               */
    int   moviendose;    /* 1 si se movió este cuadro (para la animación) */
} Jugador;

typedef struct {
    float x, z;
    EstadoBolita estado;
    int   grande;        /* 1 = bolita de poder ('o' en el mapa)          */
    float t;             /* tiempo transcurrido de la animación de comer  */
} Bolita;

typedef struct {
    EstadoJuego estado;
    int   ancho, alto;            /* tamaño de la ventana en píxeles      */

    /* Igual que Breakout: "teclas" dice si está presionada y
     * "teclasProcesadas" evita que una sola pulsación cuente muchas veces. */
    int   teclas[1024];
    int   teclasProcesadas[1024];

    /* El laberinto como texto: '#' pared, '.' bolita, 'o' bolita grande,
     * 'P' inicio de PacMan, ' ' pasillo vacío. */
    char  mapa[MAX_FILAS][MAX_COLS + 1];
    int   filas, cols;

    Jugador jugador;
    Camara  camara;

    Bolita bolitas[MAX_BOLITAS];
    int    numBolitas;
    int    bolitasRestantes;
    int    puntos;

    float tiempo;                 /* segundos desde que empezó el nivel   */
    float cierreBoca;             /* 0 = abierta ... 1 = cerrada          */
    float temporizadorMordida;    /* >0 mientras dura un "chomp"          */
    int   tituloSucio;            /* 1 = hay que actualizar el título     */

    /* Recursos de OpenGL */
    unsigned int shaderEscena, shaderBoca;
    Malla cubo, esfera, quad;
} Juego;

void Juego_Iniciar(Juego *j, int ancho, int alto);
void Juego_CargarNivel(Juego *j);           /* reinicia mapa y bolitas */
void Juego_ProcesarEntrada(Juego *j, float dt);
void Juego_Actualizar(Juego *j, float dt);
void Juego_Dibujar(Juego *j);
void Juego_Liberar(Juego *j);

#endif
