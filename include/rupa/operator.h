#ifndef RUPA_UTILS_OPERATOR_H
#define RUPA_UTILS_OPERATOR_H

#include "rupa/enum.h"
#include "rupa/structure.h"

/**
 * @brief Mendapatkan tipe operator biner dari token.
 *
 * @param token Pointer ke DataToken operator.
 * @return Enum BinaryType.
 */
BinaryType getBinaryType(DataToken *token);

/**
 * @brief Mendapatkan precedence operator.
 *
 * @param token Pointer ke DataToken operator.
 * @return Integer precedence (lebih tinggi = lebih kuat).
 */
int getPrecedence(DataToken *token);

#endif // RUPA_UTILS_OPERATOR_H
