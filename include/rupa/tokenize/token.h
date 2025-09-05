#pragma once

#include "rupa/package.h"

/**
 * findParen: mencari kurung penutup yang cocok.
 */
extern int findParen(Token *tokens, int start, int end);

/**
 * @brief Mengambil indeks terakhir dari token dalam 1 baris.
 *
 * @param token Pointer ke Token.
 * @param start Index awal.
 * @return Index akhir baris.
 */
extern int lastIndex(Token *token, int start);

/**
 * match: cek apakah token sesuai dengan tipe T.
 */
extern bool match(DataToken *data, TokenType T);

/**
 * Menentukan tipe dari token
 */
extern bool isToken(Token *t, int pos, TokenType type);

/**
 * Mendapatkan token tertentu sesuai index
 */
extern DataToken *getToken(Token *t, int pos);
