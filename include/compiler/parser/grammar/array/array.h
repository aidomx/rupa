#pragma once
#if defined(RUPA_PACKAGE_H)

/**
 * @brief Grammar array literal `[ elemen, elemen, ... ]`.
 * @return Id node array, atau GRAMMAR_NO_MATCH jika token[a] bukan awal
 *         array literal yang menutup tepat di b-1.
 */
int grammarParseArrayLiteral(struct Request *r, int a, int b);

#endif
