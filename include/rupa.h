#pragma once
#define RUPA_PACKAGE_H

/*
 * Header internal. Sengaja dipisahkan dari direktori include/ agar
 * include/ tetap berisi satu titik masuk utama:
 *
 *     #include <rupa.h>
 *
 * Pemisahan ini menjaga API publik tetap ringkas dan stabil, sedangkan
 * utilitas serta implementasi internal dapat diubah tanpa memengaruhi
 * antarmuka utama.
 */
#include "../lib/intl.h"
