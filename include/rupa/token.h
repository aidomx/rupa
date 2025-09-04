#ifndef RUPA_UTILS_TOKEN_H
#define RUPA_UTILS_TOKEN_H

#include "rupa/structure.h"

/**
 * findParen: mencari kurung penutup yang cocok.
 */
int findParen(Token *tokens, int start, int end);

/**
 * @brief Mengambil indeks terakhir dari token dalam 1 baris.
 *
 * @param token Pointer ke Token.
 * @param start Index awal.
 * @return Index akhir baris.
 */
int lastIndex(Token *token, int start);

/**
 * match: cek apakah token sesuai dengan tipe T.
 */
bool match(DataToken *data, TokenType T);

/**
 * Menentukan tipe dari token
 */
bool isToken(Token *t, int pos, TokenType type);

/**
 * Mendapatkan token tertentu sesuai index
 */
DataToken *getToken(Token *t, int pos);

#endif // RUPA_UTILS_TOKEN_H
