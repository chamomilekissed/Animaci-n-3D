/*
 * malla.h
 * ---------------------------------------------------------------------------
 * Una "malla" es la geometría de un objeto guardada en la GPU:
 *   VAO = recuerda cómo están acomodados los datos de los vértices
 *   VBO = el arreglo de vértices en la memoria de la tarjeta de video
 *   EBO = (opcional) índices para reutilizar vértices
 * Equivale a lo que el libro hace en "Hello Triangle" y en la clase
 * SpriteRenderer del juego Breakout.
 * ---------------------------------------------------------------------------
 */
#ifndef MALLA_H
#define MALLA_H

typedef struct {
    unsigned int vao, vbo, ebo;
    int numVertices;   /* se usa con glDrawArrays   (cuando no hay EBO) */
    int numIndices;    /* se usa con glDrawElements (cuando sí hay EBO) */
} Malla;

Malla Malla_CrearCubo(void);                      /* paredes y piso   */
Malla Malla_CrearEsfera(int sectores, int pilas); /* bolitas y PacMan */
Malla Malla_CrearQuadPantalla(void);              /* boca en 1a persona */
void  Malla_Dibujar(const Malla *m);
void  Malla_Liberar(Malla *m);

#endif
