#pragma once

#include "rupa/package.h"

/**
 * Mencari posisi terakhir dari RBLOCK
 */
extern int findArr(Token *tokens, int pos);

/**
 * Mencari posisi comma
 */
extern int findComma(Token *t, int start, int end);
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
 * Mendapatkan tipe token dari karakter tertentu.
 */
extern TokenType gettype(const char *ptr);

/**
 * Mendapatkan token tertentu sesuai index
 */
extern DataToken *getToken(Token *t, int pos);
