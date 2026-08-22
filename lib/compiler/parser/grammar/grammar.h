#pragma once

#if defined(RUPA_PACKAGE_H)

/*
 * Grammar bahasa Rupa dipecah menjadi unit-unit kecil di
 * src/compiler/parser/grammar/, satu file per jenis grammar, supaya
 * penambahan grammar baru ke depan tidak menumpuk pada satu file besar:
 *
 *   grammar.c              - dispatcher tipis: grammarParseStatement()
 *   grammar_shared.c        - helper level-token bersama (whitespace, dst)
 *   grammar_expression.c    - dispatcher ekspresi (array/object/call/binary)
 *   grammar_array.c         - grammar array literal `[ ... ]`
 *   grammar_object.c        - grammar object literal `{ key: value, ... }`
 *   grammar_call.c          - grammar call-expression `nama(...)`
 *   grammar_block.c         - grammar block `{ ... }` & body keyword `: ...`
 *   grammar_if.c            - grammar if/elseif/else
 *   grammar_loop.c          - grammar for/rev/while
 *   grammar_print.c         - grammar print
 *   grammar_return.c        - grammar return
 *   grammar_control.c       - grammar break/continue
 *   grammar_module.c        - grammar import/export/extends
 *   grammar_function.c      - grammar deklarasi & pemanggilan function
 *   grammar_struct.c        - grammar deklarasi struct/blueprint
 *   grammar_annotation.c    - grammar type annotation (`name: Type [=v]`)
 *   grammar_assignment.c    - grammar assignment & fallback expression
 *
 * processor.c hanya memanggil grammarParseStatement() dan
 * grammarIsWhitespace(); semua fungsi lain di header ini adalah detail
 * implementasi yang dipakai bersama ANTAR file grammar_*.c saja (pola yang
 * sama dengan lib/compiler/lexer/processor/processor.h untuk lexer).
 */

/**
 * @brief Nilai sentinel yang menandakan "grammar ini tidak cocok, coba
 *        grammar lain". Berbeda dari -1 yang berarti "grammar ini cocok,
 *        tapi gagal di-parse" (mis. node gagal dibuat).
 */
#define GRAMMAR_NO_MATCH (-2)

/* ====================== Entry point (dipakai processor.c) ============= */

/**
 * @brief Cek apakah token pada index tertentu adalah token "kosong"
 *        (NEWLINE/TAB) yang boleh dilewati saat mencari awal statement.
 */
bool grammarIsWhitespace(struct Token *t, int i);

/**
 * @brief Mem-parse satu statement berdasarkan aturan grammar bahasa Rupa
 *        mulai dari *pos hingga limit. Titik masuk utama seluruh grammar;
 *        mendelegasikan ke unit grammar_*.c yang sesuai.
 *
 * @param r Pointer ke Request parser.
 * @param pos Pointer ke posisi cursor token saat ini (akan diupdate).
 * @param limit Batas akhir (exclusive) index token yang boleh dibaca.
 * @return Index node hasil parse, atau -1 jika gagal.
 */
int grammarParseStatement(struct Request *r, int *pos, int limit);

/* ====================== Helper bersama (grammar_shared.c) ============= */

/**
 * @brief Cari akhir baris fisik (NEWLINE/ENDOF/pergantian nomor baris)
 *        mulai dari index token i.
 */
int grammarLineEnd(struct Token *t, int i);

/**
 * @brief Cari index token penutup yang berpasangan dengan token pembuka di
 *        index `open` (menghitung depth bersarang).
 */
int grammarMatchClose(struct Token *t, int open, int end, TokenType l,
                      TokenType r);

/**
 * @brief Tambahkan satu node id ke array dinamis (dipakai untuk daftar
 *        argumen, parameter, statement dalam block, dst).
 */
void grammarPushId(int **v, int *n, int x);

/* ====================== Grammar ekspresi ================================ */

/**
 * @brief Mem-parse satu ekspresi (array literal, object literal, call
 *        expression, atau binary expression sebagai fallback).
 */
int grammarParseExpr(struct Request *r, int a, int b);

/**
 * @brief Mem-parse daftar argumen/elemen yang dipisah koma pada depth 0,
 *        masing-masing di-parse lewat grammarParseExpr().
 *
 * @param out Diisi dengan array id node hasil parse (gcrealloc-owned).
 * @return Jumlah elemen.
 */
int grammarParseArgs(struct Request *r, int a, int b, int **out);

/**
 * @brief Grammar array literal `[ elemen, elemen, ... ]`.
 * @return Id node array, atau GRAMMAR_NO_MATCH jika token[a] bukan awal
 *         array literal yang menutup tepat di b-1.
 */
int grammarParseArrayLiteral(struct Request *r, int a, int b);

/**
 * @brief Grammar object literal `{ key: value, ... }`.
 * @return Id node object, atau GRAMMAR_NO_MATCH jika token[a] bukan awal
 *         object literal yang menutup tepat di b-1.
 */
int grammarParseObjectLiteral(struct Request *r, int a, int b);

/**
 * @brief Grammar call-expression `nama(arg, arg, ...)` sebagai bagian dari
 *        ekspresi (mis. di sisi kanan assignment).
 * @return Id node call, atau GRAMMAR_NO_MATCH jika token[a] bukan awal
 *         call-expression yang menutup tepat di b-1.
 */
int grammarParseCallExpr(struct Request *r, int a, int b);

/* ====================== Grammar block/body =============================== */

/**
 * @brief Mem-parse isi block `{ ... }` (dari open+1 hingga close, exclusive)
 *        menjadi daftar statement.
 */
int grammarParseBlock(struct Request *r, int open, int close);

/**
 * @brief Mem-bangun body keyword (if/for/rev/while/dst). Body `{...}`
 *        memiliki seluruh rentang brace; body `:` hanya satu baris fisik.
 *
 * @param next Diisi dengan posisi token setelah body selesai.
 */
int grammarParseKeywordBody(struct Request *r, int bodyStart, int limit,
                            int *next);

/* ====================== Grammar statement per keyword ==================== */

/**
 * @brief Grammar `if`/`elseif`/`else`.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan keyword if/elseif/else.
 */
int grammarParseIf(struct Request *r, int a, int b, int limit, int *pos);

/**
 * @brief Grammar `for`/`rev`/`while`.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan keyword for/rev/while.
 */
int grammarParseLoop(struct Request *r, int a, int b, int limit, int *pos);

/**
 * @brief Grammar `print(...)`.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan keyword print.
 */
int grammarParsePrint(struct Request *r, int a, int b, int *pos);

/**
 * @brief Grammar `return <expr>`.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan keyword return.
 */
int grammarParseReturnKeyword(struct Request *r, int a, int b, int *pos);

/** Grammar `break` dan `continue` tanpa expression. */
int grammarParseControl(struct Request *r, int a, int b, int *pos);

/**
 * @brief Grammar `import`/`export`/`extends`.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan salah satu keyword tsb.
 */
int grammarParseModule(struct Request *r, int a, int b, int *pos);

/* ====================== Grammar deklarasi ================================= */

/**
 * @brief Grammar deklarasi/pemanggilan function: `nama(...) { ... }` atau
 *        `nama(...)` sebagai call statement.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan awal pola ini.
 */
int grammarParseFunction(struct Request *r, int a, int b, int limit,
                         int *pos);

/**
 * @brief Grammar deklarasi struct/blueprint: `Nama { ... }`.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan awal pola ini.
 */
int grammarParseStruct(struct Request *r, int a, int b, int limit, int *pos);

/* ====================== Grammar annotation & assignment ==================== */

/**
 * @brief Bangun node Annotation dari satu token identifier/literal-id yang
 *        tipenya sudah dinormalisasi lexer ke field `.safetyType` (lihat
 *        processIdentifier() pada lexer, dipakai baik untuk statement
 *        `name: Type = value` maupun parameter function `name: Type`).
 *
 * @param idx Index token identifier yang membawa `.safetyType`.
 * @param valueId Id node value opsional (-1 jika tidak ada).
 * @return Id node Annotation, atau -1 jika token[idx] tidak membawa
 *         `.safetyType` (caller sebaiknya fallback ke scan literal COLON).
 */
int grammarAnnotationFromSafetyType(struct Request *r, int idx, int valueId);

/**
 * @brief Grammar annotation level-statement: `name: Type [= value]`.
 * @return GRAMMAR_NO_MATCH jika token[a] bukan awal pola ini.
 */
int grammarParseAnnotation(struct Request *r, int a, int b, int *pos);

/**
 * @brief Grammar assignment: `target = value`, dengan type opsional dari
 *        `.safetyType` target.
 * @return GRAMMAR_NO_MATCH jika tidak ada token ASSIGN di [a+1, b).
 */
int grammarParseUpdate(struct Request *r, int a, int b, int *pos);
int grammarParseAssignment(struct Request *r, int a, int b, int *pos);

/**
 * @brief Fallback akhir: ekspresi berdiri sendiri dibungkus sebagai Return
 *        node (dipakai REPL untuk echo hasil ekspresi).
 */
int grammarParseExpressionStatement(struct Request *r, int a, int b,
                                    int *pos);

#endif
