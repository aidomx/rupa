# Export (Grammar)

## NODE_EXPORT

```rupa
export a
```

```
Module statement:
  Export:
    Literal ID: a
```

Export harus berada di akhir file. Nama export adalah identifier yang digunakan saat import.

## NODE_MOD (design baru — belum aktif)

Export akan dimigrasi ke `NODE_MOD` bersama import — 1 container untuk 2 job.
Detail struct, tabel pemetaan (termasuk `export x` dengan `source == NULL` dan
policy `-> { a: private }`), dan rencana migrasi ada di
[Import (Grammar)](import.md#node_mod-design-baru--belum-aktif).

## NODE_IMPORT (referensi)

Lihat [Import (Grammar)](import.md) untuk detail AST import.
