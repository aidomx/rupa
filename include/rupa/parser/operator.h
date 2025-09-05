#pragma once

#include "rupa/package.h"
/**
 * @brief Mendapatkan tipe operator biner dari token.
 *
 * @param token Pointer ke DataToken operator.
 * @return Enum BinaryType.
 */
extern BinaryType getBinaryType(DataToken *token);

/**
 * @brief Mendapatkan precedence operator.
 *
 * @param token Pointer ke DataToken operator.
 * @return Integer precedence (lebih tinggi = lebih kuat).
 */
extern int getPrecedence(DataToken *token);
