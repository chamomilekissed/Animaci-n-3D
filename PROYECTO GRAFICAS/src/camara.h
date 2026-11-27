/*
 * camara.h
 * ---------------------------------------------------------------------------
 * Cámara basada en ángulos de Euler (yaw y pitch), como la clase Camera del
 * capítulo "Camera" del libro. Agregamos dos modos:
 *   - Primera persona: la cámara está en los "ojos" de PacMan.
 *   - Tercera persona: la cámara va detrás y arriba de PacMan.
 * ---------------------------------------------------------------------------
 */
#ifndef CAMARA_H
#define CAMARA_H

#include "matematicas.h"

typedef enum { CAM_PRIMERA_PERSONA, CAM_TERCERA_PERSONA } ModoCamara;

typedef struct {
    Vec3  posicion;      /* dónde está la cámara                         */
    Vec3  frente;        /* hacia dónde mira (vector unitario)           */
    Vec3  arribaMundo;   /* (0,1,0): el "arriba" del mundo                */
    float yaw;           /* giro izquierda/derecha, en grados            */
    float pitch;         /* inclinación arriba/abajo, en grados          */
    float sensibilidad;  /* qué tanto gira por cada píxel de mouse       */
    ModoCamara modo;
} Camara;

void Camara_Iniciar(Camara *c, float yaw);
void Camara_ProcesarMouse(Camara *c, float dx, float dy);
void Camara_ActualizarVectores(Camara *c);

/* Frente "plano": la dirección de la mirada pero sin inclinación (y = 0).
 * Sirve para caminar: PacMan no vuela aunque mires hacia arriba. */
Vec3 Camara_FrentePlano(const Camara *c);

/* Coloca la cámara según el modo y la posición del jugador y regresa
 * la matriz de vista. */
Mat4 Camara_Seguir(Camara *c, Vec3 posJugador, float alturaOjos);

#endif
