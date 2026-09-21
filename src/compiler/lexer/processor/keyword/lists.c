#include <rupa.h>

const char *keywordList[] = {
    "void",     // return-type annotation: foo(): void { }
    "if",       // Kondisi utama
    "else if",  // Kondisi lanjutan setelah if
    "else",     // Kondisi fallback
    "for",      // Loop iteratif
    "rev",      // Loop reverse (mundur)
    "while",    // Loop berbasis kondisi
    "print",    // Cetak output ke konsol
    "import",   // import modul
    "export",   // export modul
    "extends",  // extends
    "namespace", // namespace db { export ... }
    "enum",     // enum TokenType { ... }
    "const",    // const x: number = 1 — binding immutable
    "return",   // return
    "break",    // keluar dari loop
    "continue", // lanjut iterasi loop
    "async",    // mulai pekerjaan asynchronous
    "await",    // tunggu hasil asynchronous
    "case",     // pilih pattern berdasarkan subject
    "default",  // default case dalam case statement
    NULL        // Penanda akhir daftar
};

KeywordType keywordType[] = {
    /* Urutan WAJIB sama persis dengan keywordList di atas —
     * scanKeyword() memasangkan berdasarkan index. */
    KEYWORD_VOID,      // void
    KEYWORD_IF,        // if
    KEYWORD_ELSEIF,    // else if
    KEYWORD_ELSE,      // else
    KEYWORD_FOR,       // for
    KEYWORD_REV,       // rev
    KEYWORD_WHILE,     // while
    KEYWORD_PRINT,     // print
    KEYWORD_IMPORT,    // import
    KEYWORD_EXPORT,    // export
    KEYWORD_EXTENDS,   // extends
    KEYWORD_NAMESPACE, // namespace
    KEYWORD_ENUM,      // enum
    KEYWORD_CONST,     // const
    KEYWORD_RETURN,    // return
    KEYWORD_BREAK,     // break
    KEYWORD_CONTINUE,  // continue
    KEYWORD_ASYNC,     // async
    KEYWORD_AWAIT,     // await
    KEYWORD_CASE,      // case
    KEYWORD_DEFAULT,   // default
    KEYWORD_NULL       // sentinel
};

const int keywordListSize = sizeof(keywordList) / sizeof(keywordList[0]) - 1;
