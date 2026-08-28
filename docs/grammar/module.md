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
| os | Akses sistem operasi | [module/os.md](module/os.md) |

## Menambah Module Baru

Untuk menambah module baru ke dalam rupa:

1. Buat file di `module/nama_module.md`
2. Dokumentasikan: AST node, value types, dependency
3. Tambah entry ke tabel di atas
