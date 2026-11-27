/*
 * matematicas.h
 * ---------------------------------------------------------------------------
 * Mini biblioteca de matemáticas 3D (vectores y matrices 4x4).
 *
 * El libro LearnOpenGL usa GLM, pero GLM es una biblioteca de C++ (usa
 * clases y plantillas), así que no se puede usar desde C. Aquí escribimos
 * solo lo que necesitamos: vectores de 3 componentes y matrices 4x4.
 *
 * ALTERNATIVA: "cglm" (https://github.com/recp/cglm) es una versión de GLM
 * hecha para C. Es más completa y rápida (usa instrucciones SIMD), pero es
 * una dependencia más que hay que descargar y compilar. Preferimos escribir
 * estas ~150 líneas porque así se entiende exactamente qué hace cada matriz,
 * que es justo lo que se evalúa en una materia de Gráficas por computadora.
 *
 * IMPORTANTE: las matrices se guardan en "column-major" (por columnas), que
 * es el orden que espera OpenGL. El elemento (fila r, columna c) está en
 * m[c * 4 + r]. Por eso al mandarlas al shader usamos GL_FALSE en
 * glUniformMatrix4fv (no hay que transponerlas).
 * ---------------------------------------------------------------------------
 */
#ifndef MATEMATICAS_H
#define MATEMATICAS_H

#include <math.h>

#define PI_F 3.14159265358979f

/* ------------------------------ Vectores --------------------------------- */

typedef struct { float x, y, z; } Vec3;

/* "static inline": el compilador copia la función donde se usa (más rápido)
 * y evita errores de "definición duplicada" al incluir el .h en varios .c */
static inline Vec3 v3(float x, float y, float z) { Vec3 r = { x, y, z }; return r; }

static inline Vec3 v3_suma(Vec3 a, Vec3 b)      { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline Vec3 v3_resta(Vec3 a, Vec3 b)     { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline Vec3 v3_escala(Vec3 a, float s)   { return v3(a.x * s, a.y * s, a.z * s); }
static inline float v3_punto(Vec3 a, Vec3 b)    { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float v3_longitud(Vec3 a)         { return sqrtf(v3_punto(a, a)); }

/* Producto cruz: da un vector perpendicular a "a" y a "b".
 * Lo usamos para sacar el vector "derecha" de la cámara. */
static inline Vec3 v3_cruz(Vec3 a, Vec3 b)
{
    return v3(a.y * b.z - a.z * b.y,
              a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}

/* Normalizar = dejar el vector con longitud 1 conservando su dirección. */
static inline Vec3 v3_normalizar(Vec3 a)
{
    float l = v3_longitud(a);
    if (l < 1e-6f) return v3(0.0f, 0.0f, 0.0f);   /* evita dividir entre cero */
    return v3_escala(a, 1.0f / l);
}

/* Interpolación lineal: t=0 da "a", t=1 da "b", t=0.5 da el punto medio. */
static inline Vec3 v3_mezclar(Vec3 a, Vec3 b, float t)
{
    return v3_suma(a, v3_escala(v3_resta(b, a), t));
}

static inline float radianes(float grados) { return grados * PI_F / 180.0f; }

static inline float limitar(float v, float min, float max)
{
    return v < min ? min : (v > max ? max : v);
}

/* ------------------------------ Matrices --------------------------------- */

typedef struct { float m[16]; } Mat4;

static inline Mat4 m4_identidad(void)
{
    Mat4 r = { { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 } };
    return r;
}

/* Multiplicación de matrices: r = a * b.
 * Ojo: el orden importa. Al combinar transformaciones se leen de derecha a
 * izquierda: trasladar * rotar * escalar = primero escala, luego rota y al
 * final traslada (igual que en el capítulo "Transformations" del libro). */
static inline Mat4 m4_multiplicar(Mat4 a, Mat4 b)
{
    Mat4 r;
    for (int c = 0; c < 4; c++)          /* columna del resultado */
        for (int f = 0; f < 4; f++) {    /* fila del resultado    */
            float suma = 0.0f;
            for (int k = 0; k < 4; k++)
                suma += a.m[k * 4 + f] * b.m[c * 4 + k];
            r.m[c * 4 + f] = suma;
        }
    return r;
}

/* Matriz de traslación: mueve el objeto a la posición (x, y, z).
 * La traslación vive en la última columna (índices 12, 13, 14). */
static inline Mat4 m4_trasladar(Vec3 t)
{
    Mat4 r = m4_identidad();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

/* Matriz de escala: hace el objeto más grande o más chico en cada eje. */
static inline Mat4 m4_escalar(Vec3 s)
{
    Mat4 r = m4_identidad();
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
    return r;
}

/* Rotación alrededor del eje Y (el eje vertical). Ángulo en radianes.
 * ALTERNATIVA: una rotación sobre un eje cualquiera (fórmula de Rodrigues,
 * como glm::rotate) o cuaterniones. Aquí solo giramos a PacMan sobre el
 * piso, así que con el eje Y basta y la fórmula es mucho más corta. */
static inline Mat4 m4_rotarY(float ang)
{
    Mat4 r = m4_identidad();
    float c = cosf(ang), s = sinf(ang);
    r.m[0] = c;  r.m[8]  = s;
    r.m[2] = -s; r.m[10] = c;
    return r;
}

/* Proyección en perspectiva (igual que glm::perspective).
 * Hace que lo lejano se vea más chico. "fovy" es el campo de visión
 * vertical en radianes; cerca/lejos son los planos de recorte. */
static inline Mat4 m4_perspectiva(float fovy, float aspecto, float cerca, float lejos)
{
    Mat4 r = { { 0 } };
    float f = 1.0f / tanf(fovy * 0.5f);
    r.m[0]  = f / aspecto;
    r.m[5]  = f;
    r.m[10] = (lejos + cerca) / (cerca - lejos);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * lejos * cerca) / (cerca - lejos);
    return r;
}

/* Matriz de vista "LookAt" (igual que glm::lookAt, capítulo "Camera").
 * Coloca la cámara en "ojo" mirando hacia "objetivo". Construye una base
 * con 3 vectores perpendiculares (derecha, arriba, frente) y además mueve
 * el mundo al revés de donde está la cámara. */
static inline Mat4 m4_mirarA(Vec3 ojo, Vec3 objetivo, Vec3 arriba)
{
    Vec3 f = v3_normalizar(v3_resta(objetivo, ojo));  /* hacia dónde ve  */
    Vec3 s = v3_normalizar(v3_cruz(f, arriba));        /* derecha         */
    Vec3 u = v3_cruz(s, f);                            /* arriba real     */

    Mat4 r = m4_identidad();
    r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;
    r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -v3_punto(s, ojo);
    r.m[13] = -v3_punto(u, ojo);
    r.m[14] =  v3_punto(f, ojo);
    return r;
}

#endif /* MATEMATICAS_H */
