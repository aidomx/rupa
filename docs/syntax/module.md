# Module

Module memungkinkan kode untuk diorganisir dan digunakan kembali dengan mengimpor dan mengekspor deklarasi antar file.

- [Import](import.md) — mengimpor kode dari file lain
- [Export](export.md) — mengekspor kode agar bisa di-import

## Path Resolution

| Path | Resolves to |
|------|-------------|
| `./modules.a` | `./modules/a.rp` |
| `./modules` | `./modules/*.rp` |
| `rupa` | stdlib module |
