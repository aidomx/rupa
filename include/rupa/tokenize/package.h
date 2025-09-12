#pragma once

#include "rupa/package.h"

#include "rupa/tokenize/factory.h"
#include "rupa/tokenize/helper.h"
#include "rupa/tokenize/processor.h"
#include "rupa/tokenize/token.h"

/**
 * @brief Menambahkan delimiter sebagai token.
 *
 * @param tokens Pointer ke struktur token.
 * @param c Pointer character token
 * @param safetyType token
 * @param line token
 * @param row token
 * @return Status keberhasilan.
 */
extern int addDelim(Token *tokens, char c, char *safetyType, int line, int row);

/**
 * @brief Menambahkan token baru ke dalam struktur token.
 *
 * @param tokens Pointer ke struktur token.
 * @param data Pointer ke data token.
 * @return Status keberhasilan.
 */
extern int addToken(Token *tokens, DataToken data);

/**
 * @brief Menghapus seluruh token dan mengosongkan buffer.
 *
 * @param token Pointer ke struktur token.
 * @param capacity Kapasitas awal token.
 */
extern void clearToken(Token *token, int capacity);

/**
 * @brief Melakukan tokenisasi terhadap input tertentu.
 *
 * @param state ReplState
 */
extern Token *tokenize(Token *tokens, char **history, int length, int line);
