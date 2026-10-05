#pragma once
#if defined(RUPA_PACKAGE_H)

/**
 * @brief Mendapatkan tipe operator biner dari token.
 *
 * @param token Pointer ke DataToken operator.
 * @return Enum BinaryType.
 */
extern enum BinaryType getBinaryType(struct DataToken *token);

/**
 * @brief Mendapatkan precedence operator.
 *
 * @param token Pointer ke DataToken operator.
 * @return Integer precedence (lebih tinggi = lebih kuat).
 */
extern int getPrecedence(struct DataToken *token);

#endif
