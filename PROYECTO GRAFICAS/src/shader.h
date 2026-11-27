/*
 * shader.h
 * ---------------------------------------------------------------------------
 * Equivalente en C de la clase "Shader" del juego Breakout del libro.
 * En C no hay clases, así que usamos funciones con el prefijo "Shader_" que
 * reciben el identificador (ID) del programa de shaders.
 * ---------------------------------------------------------------------------
 */
#ifndef SHADER_H
#define SHADER_H

#include "matematicas.h"

/* Compila un vertex shader y un fragment shader, los enlaza en un programa
 * y regresa su ID (0 si hubo error). */
unsigned int Shader_Compilar(const char *codigoVertice, const char *codigoFragmento);

/* "Usar" = activar el programa para los siguientes dibujos. */
void Shader_Usar(unsigned int programa);

/* Funciones para mandar datos ("uniforms") del CPU al shader. */
void Shader_SetMat4 (unsigned int programa, const char *nombre, const Mat4 *m);
void Shader_SetVec3 (unsigned int programa, const char *nombre, Vec3 v);
void Shader_SetFloat(unsigned int programa, const char *nombre, float v);

#endif
