/*
 * camara.c
 * ---------------------------------------------------------------------------
 * Implementación de la cámara (capítulo "Camera" del libro).
 * ---------------------------------------------------------------------------
 */
#include "camara.h"

void Camara_Iniciar(Camara *c, float yaw)
{
    c->posicion     = v3(0.0f, 0.0f, 0.0f);
    c->arribaMundo  = v3(0.0f, 1.0f, 0.0f);
    c->yaw          = yaw;
    c->pitch        = 0.0f;
    c->sensibilidad = 0.1f;          /* mismo valor que usa el libro */
    c->modo         = CAM_PRIMERA_PERSONA;
    Camara_ActualizarVectores(c);
}

/* Convierte el movimiento del mouse en giro de la cámara. */
void Camara_ProcesarMouse(Camara *c, float dx, float dy)
{
    c->yaw   += dx * c->sensibilidad;
    c->pitch += dy * c->sensibilidad;

    /* Limitamos el pitch para no "dar la vuelta" mirando hacia arriba o
     * abajo: a 90° exactos el vector frente queda paralelo a "arriba" y el
     * producto cruz de lookAt se vuelve cero (la imagen se voltea).
     * En PacMan basta con poder mirar un poco arriba/abajo. */
    c->pitch = limitar(c->pitch, -45.0f, 45.0f);

    Camara_ActualizarVectores(c);
}

/* Calcula el vector "frente" a partir de los ángulos (fórmula del libro).
 *   x = cos(yaw) * cos(pitch)
 *   y = sin(pitch)
 *   z = sin(yaw) * cos(pitch)
 * ALTERNATIVA: guardar la orientación como cuaternión evita el "gimbal
 * lock", pero para una cámara que no rueda (sin "roll") Euler es suficiente
 * y mucho más fácil de explicar. */
void Camara_ActualizarVectores(Camara *c)
{
    float y = radianes(c->yaw), p = radianes(c->pitch);
    c->frente = v3_normalizar(v3(cosf(y) * cosf(p), sinf(p), sinf(y) * cosf(p)));
}

Vec3 Camara_FrentePlano(const Camara *c)
{
    float y = radianes(c->yaw);
    return v3(cosf(y), 0.0f, sinf(y));
}

Mat4 Camara_Seguir(Camara *c, Vec3 posJugador, float alturaOjos)
{
    if (c->modo == CAM_PRIMERA_PERSONA) {
        /* Los ojos están dentro de PacMan: misma x,z, a la altura de los ojos.
         * Miramos hacia posicion + frente (igual que en el libro). */
        c->posicion = v3(posJugador.x, alturaOjos, posJugador.z);
        return m4_mirarA(c->posicion, v3_suma(c->posicion, c->frente), c->arribaMundo);
    } else {
        /* Tercera persona: nos ponemos 2.2 unidades detrás y 1.6 arriba,
         * y miramos hacia PacMan.
         * ALTERNATIVA: una cámara cenital (desde arriba, como el PacMan
         * original) se haría con posicion = jugador + (0, 12, 0.01). La de
         * atrás se siente más "3D" y deja ver la boca de PacMan. */
        Vec3 objetivo = v3(posJugador.x, alturaOjos, posJugador.z);
        Vec3 atras = v3_escala(Camara_FrentePlano(c), -2.2f);
        c->posicion = v3_suma(v3_suma(objetivo, atras), v3(0.0f, 1.6f, 0.0f));
        return m4_mirarA(c->posicion, objetivo, c->arribaMundo);
    }
}
