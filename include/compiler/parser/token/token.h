#pragma once

#if defined(RUPA_PACKAGE_H)

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
extern int addDelim(struct Token *tokens, char c, char *safetyType, int line,
                    int row);

/**
 * @brief Menambahkan token baru ke dalam struktur token.
 *
 * @param tokens Pointer ke struktur token.
 * @param data Pointer ke data token.
 * @return Status keberhasilan.
 */
extern int addToken(struct Token *tokens, struct DataToken data);

/**
 * @brief Menghapus seluruh token dan mengosongkan buffer.
 *
 * @param token Pointer ke struktur token.
 * @param capacity Kapasitas awal token.
 */
extern void clearToken(struct Token *token, int capacity);

extern struct DataToken createDataToken(char *input, char *safetyType,
                                        enum TokenType tokenType, int line,

                                        int row);

/**
 * @brief Membuat struktur token baru.
 *
 * @param capacity Jumlah awal kapasitas token.
 * @return Pointer ke Token.
 */
extern struct Token *createToken(int capacity);

extern int createTokenId(struct State *state, int start, int end);

/**
 * Mencari posisi terakhir dari RBLOCK
 */
extern int findArr(struct Token *tokens, int pos);

/**
 * Mencari posisi comma
 */
extern int findComma(struct Token *t, int start, int end);
/**
 * findParen: mencari kurung penutup yang cocok.
 */
extern int findParen(struct Token *tokens, int start, int end);

/**
 * @brief Mengambil indeks terakhir dari token dalam 1 baris.
 *
 * @param token Pointer ke Token.
 * @param start Index awal.
 * @return Index akhir baris.
 */
extern int lastIndex(struct Token *token, int start);

/**
 * match: cek apakah token sesuai dengan tipe T.
 */
extern bool match(struct DataToken *data, enum TokenType T);

/**
 * Menentukan tipe dari token
 */
extern bool isToken(struct Token *token, int pos, enum TokenType type);

/**
 * Mengatur type token dari character
 */
extern enum TokenType setTokenType(const char *ptr);

/**
 * Mendapatkan tipe token dari karakter tertentu.
 */
extern enum TokenType gettype(const char *ptr);

/**
 * Mendapatkan token tertentu sesuai index
 */
extern struct DataToken *getToken(struct Token *t, int pos);

extern int processToken(struct State *state);

/**
 * @brief Menyimpan token berdasarkan LexerState aktif.
 * @param lex Pointer ke LexerState
 * @return index token atau -1 jika gagal
 */
extern int saveToken(LexerState *lex);

extern int stateTokenContext(struct State *state, int start, int end);

/**
 * @brief Melakukan tokenisasi terhadap input tertentu.
 *
 * @param state ReplState
 */
extern struct Token *tokenize(struct Token *tokens, char **history, int length,
                              int line);

#endif
