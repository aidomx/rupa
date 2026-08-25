#include <rupa.h>

const char *keywordList[] = {
    "if",      // Kondisi utama
    "elseif",  // Kondisi lanjutan setelah if
    "else",    // Kondisi fallback
    "for",     // Loop iteratif
    "rev",     // Loop reverse (mundur)
    "while",   // Loop berbasis kondisi
    "print",   // Cetak output ke konsol
    "import",  // import modul
    "export",  // export modul
    "extends", // extends
    "return",  // return
    "break",   // keluar dari loop
    "continue",// lanjut iterasi loop
    "async",   // mulai pekerjaan asynchronous
    "await",   // tunggu hasil asynchronous
    NULL       // Penanda akhir daftar
};

KeywordType keywordType[] = {
    KEYWORD_IF,      // if
    KEYWORD_ELSEIF,  // elseif
    KEYWORD_ELSE,    // else
    KEYWORD_FOR,     // for
    KEYWORD_REV,     // rev
    KEYWORD_WHILE,   // while
    KEYWORD_PRINT,   // print
    KEYWORD_IMPORT,  // import
    KEYWORD_EXPORT,  // export
    KEYWORD_EXTENDS, // extends
    KEYWORD_RETURN,  // return
    KEYWORD_BREAK,   // break
    KEYWORD_CONTINUE,// continue
    KEYWORD_ASYNC,   // async
    KEYWORD_AWAIT,   // await
    KEYWORD_NULL     // sentinel
};

const int keywordListSize = sizeof(keywordList) / sizeof(keywordList[0]) - 1;
