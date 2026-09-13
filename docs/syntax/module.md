# Module

Module memungkinkan kode untuk diorganisir dan digunakan kembali dengan mengimpor dan mengekspor deklarasi antar file.

- [Import](import.md) — mengimpor kode dari file lain
- [Export](export.md) — mengekspor kode agar bisa di-import

## Namespace

Namespace adalah entry point bernama untuk sekumpulan export. Namespace dapat
meratakan isi module dengan wildcard `.*`.

```rupa
namespace db {
  export driver.*
}

import db from rupa

db.use(...)
```

Pola akses:

```text
ns.ns    → namespace di dalam namespace
ns.props → property/function langsung pada namespace
```

## Path Resolution

| Path | Resolves to |
|------|-------------|
| `./modules.a` | `./modules/a.rp` |
| `./modules` | `./modules/*.rp` |
| `rupa` | Root namespace package Rupa dari archive project/global/system |

## Module Docs

| Module | Docs |
|--------|------|
| Math | [syntax](../modules/syntax/math.md) · [grammar](../modules/grammar/math.md) |
| OS | [syntax](../modules/syntax/os.md) · [grammar](../modules/grammar/os.md) |
| IO | [syntax](../modules/syntax/io.md) · [grammar](../modules/grammar/io.md) |
| JSON | [syntax](../modules/syntax/json.md) · [grammar](../modules/grammar/json.md) |
| String | [syntax](../modules/syntax/string.md) · [grammar](../modules/grammar/string.md) |
| Thread | [syntax](../modules/syntax/thread.md) · [grammar](../modules/grammar/thread.md) |
| HTTP | [syntax](../modules/syntax/http.md) · [grammar](../modules/grammar/http.md) |
