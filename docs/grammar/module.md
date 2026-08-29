# Module Grammar

Module grammar mengenali keyword:

```text
import
export
extends
```

Source test saat ini:

```rupa
import sys from rupa
import sys, render, resources from rupa
import rupa.system as sys
```

Parser membuat node module dengan jenis node berdasarkan keyword:

```text
NODE_IMPORT
NODE_EXPORT
NODE_EXTENDS
```

## Daftar Module

Dokumentasi grammar untuk setiap module tersedia di subdirektori ini:

| Module | Deskripsi | File |
|--------|-----------|------|
| os | Akses sistem operasi | [modules/os.md](modules/os.md) |
| io | Grammar module input terminal | [modules/io.md](modules/io.md) |
| json | Grammar module JSON | [modules/json.md](modules/json.md) |
| math | Grammar module matematika | [modules/math.md](modules/math.md) |
| string | Grammar module string | [modules/string.md](modules/string.md) |

## Menambah Module Baru

Untuk menambah module baru ke dalam rupa:

1. Buat file di `modules/nama_module.md`
2. Dokumentasikan: AST node, value types, dependency
3. Tambah entry ke tabel di atas
