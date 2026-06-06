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
    KEYWORD_EXPORT,  // export
    KEYWORD_EXTENDS, // extends
    KEYWORD_IMPORT,  // import
    KEYWORD_RETURN,  // return
    KEYWORD_NULL     // null
};

const int keywordListSize = sizeof(keywordList) / sizeof(keywordList[0]) - 1;
