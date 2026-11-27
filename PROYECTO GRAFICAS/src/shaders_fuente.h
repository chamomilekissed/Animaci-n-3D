/*
 * shaders_fuente.h
 * ---------------------------------------------------------------------------
 * Código GLSL de los shaders, guardado como cadenas de texto de C.
 *
 * ALTERNATIVA: el libro guarda cada shader en un archivo .vs / .fs y los lee
 * con un ResourceManager. Lo preferimos dentro del código porque así el
 * programa funciona sin importar desde qué carpeta se ejecute (un error muy
 * común es "no encuentra el archivo del shader" y pantalla negra). La
 * desventaja: si cambias el shader hay que recompilar el programa.
 *
 * Truco de C: cadenas seguidas como "abc" "def" se unen solas en "abcdef".
 * El "\n" al final de cada línea ayuda a que los errores de GLSL indiquen el
 * número de línea correcto.
 * ---------------------------------------------------------------------------
 */
#ifndef SHADERS_FUENTE_H
#define SHADERS_FUENTE_H

/* ============================ SHADER DE ESCENA ============================
 * Dibuja paredes, piso, bolitas y a PacMan (en tercera persona) con
 * iluminación Phong (capítulos "Basic Lighting" y "Light casters").      */

static const char *VERTICE_ESCENA =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"      /* posición del vértice   */
    "layout (location = 1) in vec3 aNormal;\n"   /* normal (hacia afuera)  */
    "uniform mat4 modelo;\n"
    "uniform mat4 vista;\n"
    "uniform mat4 proyeccion;\n"
    "out vec3 vPosMundo;\n"   /* posición en el mundo, para la luz        */
    "out vec3 vNormal;\n"
    "out vec3 vLocal;\n"      /* posición sin transformar, para la boca   */
    "void main() {\n"
    "    vec4 mundo = modelo * vec4(aPos, 1.0);\n"
    "    vPosMundo = mundo.xyz;\n"
    /* Solo usamos escalas uniformes (iguales en x, y, z), así que basta
     * con mat3(modelo). El libro usa transpose(inverse(modelo)), que es
     * lo correcto con escalas NO uniformes, pero cuesta más calcularlo.   */
    "    vNormal = mat3(modelo) * aNormal;\n"
    "    vLocal = aPos;\n"
    "    gl_Position = proyeccion * vista * mundo;\n"
    "}\n";

static const char *FRAGMENTO_ESCENA =
    "#version 330 core\n"
    "in vec3 vPosMundo;\n"
    "in vec3 vNormal;\n"
    "in vec3 vLocal;\n"
    "uniform vec3  uColor;\n"       /* color base del objeto              */
    "uniform vec3  uLuzPos;\n"      /* la luz viaja con PacMan            */
    "uniform vec3  uVistaPos;\n"    /* posición de la cámara              */
    "uniform float uEmision;\n"     /* >0 = el objeto brilla solo         */
    "uniform float uBoca;\n"        /* >0 = recortar boca de PacMan       */
    "uniform vec3  uColorNiebla;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    /* --- Boca de PacMan: descartamos los píxeles dentro de una 'cuña'.
     * La cuña mira hacia +X en coordenadas locales de la esfera. ------- */
    "    if (uBoca > 0.0 && vLocal.x > 0.0) {\n"
    "        float ang = atan(vLocal.y, vLocal.x);\n"
    "        if (abs(ang) < uBoca) discard;\n"
    "    }\n"
    "    vec3 N = normalize(vNormal);\n"
    "    vec3 base = uColor;\n"
    /* Si vemos la cara de atrás de PacMan (su interior, a través de la
     * boca), la pintamos rojo oscuro y volteamos la normal.               */
    "    if (uBoca > 0.0 && !gl_FrontFacing) { N = -N; base = vec3(0.35, 0.05, 0.05); }\n"
    "    vec3 L = uLuzPos - vPosMundo;\n"
    "    float d = length(L);\n"
    "    L = L / d;\n"
    "    vec3 V = normalize(uVistaPos - vPosMundo);\n"
    "    vec3 R = reflect(-L, N);\n"
    /* Atenuación: la luz se debilita con la distancia (tabla del capítulo
     * 'Light casters', valores para un alcance de ~50 unidades).         */
    "    float aten = 1.0 / (1.0 + 0.09 * d + 0.032 * d * d);\n"
    "    float ambiente = 0.18;\n"
    "    float difusa   = max(dot(N, L), 0.0);\n"
    "    float especular = pow(max(dot(V, R), 0.0), 32.0) * 0.35;\n"
    "    vec3 color = base * (ambiente + difusa * aten) + vec3(especular * aten)\n"
    "               + base * uEmision;\n"
    /* Niebla: lo lejano se funde con el fondo y da profundidad al laberinto. */
    "    float dist = length(uVistaPos - vPosMundo);\n"
    "    float niebla = clamp((dist - 3.0) / 12.0, 0.0, 1.0);\n"
    "    FragColor = vec4(mix(color, uColorNiebla, niebla), 1.0);\n"
    "}\n";

/* ============================ SHADER DE BOCA ==============================
 * Superposición 2D en primera persona: dibuja las "mandíbulas" amarillas de
 * PacMan en la parte de arriba y de abajo de la pantalla. Se cierran cuando
 * come una bolita, así se ve el "chomp" desde adentro.
 * Usa un cuadrado que cubre toda la pantalla (coordenadas NDC de -1 a 1),
 * parecido al quad de post-procesamiento del juego Breakout.             */

static const char *VERTICE_BOCA =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "out vec2 vPos;\n"
    "void main() {\n"
    "    vPos = aPos;\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"  /* ya está en NDC */
    "}\n";

static const char *FRAGMENTO_BOCA =
    "#version 330 core\n"
    "in vec2 vPos;\n"
    "uniform float uCierre;\n"   /* 0 = boca abierta, 1 = cerrada */
    "out vec4 FragColor;\n"
    "void main() {\n"
    /* El borde de cada mandíbula es una curva (parábola) que baja desde
     * arriba (y sube desde abajo) según uCierre. Usamos abs(y) para que
     * una sola fórmula sirva para las dos mandíbulas (son simétricas).   */
    "    float borde = mix(1.2, 0.0, uCierre) + 0.25 * vPos.x * vPos.x;\n"
    "    float d = abs(vPos.y) - borde;\n"
    "    if (d < 0.0) discard;\n"                /* fuera de la mandíbula */
    "    vec3 amarillo = vec3(1.0, 0.85, 0.0);\n"
    /* Más claro en el 'labio' y más oscuro hacia adentro: da volumen.    */
    "    vec3 color = mix(amarillo * 1.1, amarillo * 0.4, smoothstep(0.0, 0.5, d));\n"
    "    FragColor = vec4(color, 0.93);\n"
    "}\n";

#endif
