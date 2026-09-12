# Comment

## Apa yang bisa ditulis?

```rupa
# comment
// comment
/* comment */
/**
 * multiline
 * comment
 */
```

## Kapan digunakan?

Gunakan comment untuk menandai program yang diabaikan untuk diproses oleh sistem.

## Apa hasilnya?

Semua jenis comment (`#`, `//`, `/* */`) diparse menjadi token oleh lexer,
lalu diabaikan oleh interpreter. Comment tetap tersimpan di AST sehingga
bisa digunakan oleh formatter atau tools lain.

## Tipe Comment

| Syntax | Tipe | Contoh |
|--------|------|--------|
| `#` | Inline (hash) | `# TODO: fix ini` |
| `//` | Inline (slash) | `// untuk debugging` |
| `/* */` | Block | `/* penjelasan */` |
| `/** */` | Block (dokumentasi) | `/** doc comment */` |
