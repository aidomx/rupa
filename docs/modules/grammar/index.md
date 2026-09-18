# Module Grammar

Dokumentasi grammar & implementasi untuk modul bawaan Rupa: struktur AST
yang dibentuk parser, bentuk module object, dan catatan implementasi
native. Untuk dokumentasi penggunaan per modul, lihat
[Modules](../syntax/index.md).

## Pola umum

Semua import modul mengikuti satu pola grammar:

```text
import os from rupa      → NODE_MOD (chain)
import math from rupa
import { join } from rupa.sys
```

Interpreter menginisialisasi modul dan mendaftarkan fungsinya sebagai
`VALUE_NATIVE_FUNCTION` (native) atau object namespace bahasa Rupa
(mis. `math`, `sys`, `fs` yang ditulis di `stdlib/`). Detail mekanisme:
[Grammar — Module](../../grammar/module.md).

## Grammar per modul

| Modul | Grammar | Catatan implementasi |
|-------|---------|----------------------|
| [os](os.md) | import statement, module object, native fn | akses sistem POSIX |
| [io](io.md) | — | input/toNumber |
| [json](json.md) | — | stringify/parse |
| [string](string.md) | — | manipulasi string |
| [math](math.md) | — | ditulis bahasa Rupa (stdlib) |
| [thread](thread.md) | AST + module structure | `thread.create()` hanya menerima native fn |
| [http](http.md) | AST + module structure | server blocking; handler via queue thread-safe |
| [datetime](datetime.md) | AST + module structure | waktu lokal & format |
| [regex](regex.md) | AST + module structure | match/test/replace/groups |
| [crypto](crypto.md) | AST + module structure | hash/hmac (OpenSSL) |
| [net](net.md) | AST + module structure | TCP client/server |

Modul `sys`, `fs`, `database` ditulis murni dengan bahasa Rupa di atas
modul native (`os`, `fsbase`) sehingga tidak punya grammar khusus —
mereka package biasa dengan `export` (lihat
[Syntax — Export](../../syntax/export.md)).
