# Grammar: Comment

## Tokenisasi

Lexer memproduksi token untuk semua jenis comment. Comment tidak diabaikan
di lexer — mereka diparse menjadi token COMMENT dan disimpan di aliran token.

### Inline Comment (`#` atau `//`)

```
HASHTAG  COMMENT  →  NODE_INLINE_COMMENT
SLASH    COMMENT  →  NODE_INLINE_COMMENT   (ketika text dimulai "//")
```

### Block Comment (`/* */`)

```
SLASH    COMMENT  →  NODE_BLOCK_COMMENT    (ketika text dimulai "/*")
```

## AST Node

```c
struct AstComment {
  int type;       // NODE_INLINE_COMMENT atau NODE_BLOCK_COMMENT
  char *value;    // isi text comment (tanpa marker pembuka)
};
```

## Interpretasi

Interpreter mengabaikan semua comment nodes — mengembalikan `valueNull()`.
