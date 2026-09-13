# TODO — Rupa Language

> Dokumentasi status semua fitur: yang sudah siap, yang sedang dikerjakan, dan yang belum tersedia.
> Terakhir diperbarui: 13 September 2026

---

## Ringkasan Status

| Kategori         | Siap | Dalam Pengembangan | Belum |
| ---------------- | ---- | ------------------ | ----- |
| Syntax & Grammar | 26   | 1                  | 6     |
| Standard Library | 16   | 0                  | 1     |
| Module System    | 10   | 0                  | 3     |
| REPL & Editor    | 14   | 1                  | 0     |
| Compiler (`-c`)  | 0    | 0                  | 1     |
| Testing          | 45   | -                  | 0     |
| Documentation    | 30   | 0                  | 2     |

> Sumber angka: tabel di tiap bagian dokumen ini. Testing = test suite syntax
> (45/45 PASS via `./build.sh test`); tambahan: 29 file test execution di
> `tests/execution/` (via `--test-exec`, termasuk `array_methods`,
> `string_methods`, `sys_fs_db`, `sys_module`) dan scan formatter
> `./build.sh fmt -` atas 110 file: 109 PASS, 1 FAIL by-design
> (`tests/stress/index.rp` yang memang mengharapkan LexerError).
> Standard Library 16 = 11 modul native (C) + 5 Rupa packages (sys, fs,
> database, collections, strings). REPL & Editor 14 = 8 fitur formatter + 6 fitur REPL.
> Documentation 30 mengikuti tabel bagian 7.

---

## 1. Syntax & Grammar

### ✅ Sudah Siap

| Fitur            | Syntax Docs                 | Grammar Docs                 | Status |
| ---------------- | --------------------------- | ---------------------------- | ------ |
| Literal          | `docs/syntax/literal.md`    | `docs/grammar/literal.md`    | ✅     |
| Print            | `docs/syntax/print.md`      | `docs/grammar/print.md`      | ✅     |
| Assignment       | `docs/syntax/assignment.md` | `docs/grammar/assignment.md` | ✅     |
| Expression       | `docs/syntax/expression.md` | `docs/grammar/expression.md` | ✅     |
| String           | `docs/syntax/string.md`     | `docs/grammar/string.md`     | ✅     |
| Array            | `docs/syntax/array.md`      | `docs/grammar/array.md`      | ✅     |
| Object           | `docs/syntax/object.md`     | `docs/grammar/object.md`     | ✅     |
| If/Else          | `docs/syntax/if.md`         | `docs/grammar/if.md`         | ✅     |
| Block            | `docs/syntax/block.md`      | `docs/grammar/block.md`      | ✅     |
| Function         | `docs/syntax/function.md`   | `docs/grammar/function.md`   | ✅     |
| Return           | `docs/syntax/return.md`     | `docs/grammar/return.md`     | ✅     |
| Loop (while)     | `docs/syntax/loop.md`       | `docs/grammar/loop.md`       | ✅     |
| Case             | `docs/syntax/case.md`       | `docs/grammar/case.md`       | ✅     |
| Call             | `docs/syntax/call.md`       | `docs/grammar/call.md`       | ✅     |
| Struct           | `docs/syntax/struct.md`     | `docs/grammar/struct.md`     | ✅     |
| Annotation       | `docs/syntax/annotation.md` | `docs/grammar/annotation.md` | ✅     |
| Update/Increment | `docs/syntax/update.md`     | `docs/grammar/update.md`     | ✅     |
| Module Import    | `docs/syntax/import.md`     | `docs/grammar/import.md`     | ✅     |
| Module Export    | `docs/syntax/export.md`     | `docs/grammar/export.md`     | ✅     |
| Control Flow     | `docs/syntax/control.md`    | `docs/grammar/control.md`    | ✅     |
| Fallback         | `docs/syntax/fallback.md`   | `docs/grammar/fallback.md`   | ✅     |
| Then (inline)    | `docs/syntax/then.md`       | `docs/grammar/then.md`       | ✅     |
| Member Access    | `docs/syntax/struct.md`     | `docs/grammar/member.md`     | ✅     |
| Comment          | `docs/syntax/comment.md`    | `docs/grammar/comment.md`    | ✅     |

### 🔨 Dalam Pengembangan

| Fitur       | Syntax Docs                   | Grammar Docs                   | Status | Catatan                                                  |
| ----------- | ----------------------------- | ------------------------------ | ------ | -------------------------------------------------------- |
| Async/Await | `docs/syntax/async.md`        | `docs/grammar/async.md`        | 🔨     | `await` syntax & handler blok belum stabil               |
| HTTP        | `docs/modules/syntax/http.md` | `docs/modules/grammar/http.md` | ✅     | Handler user-defined works via queue-based thread safety |

### ❌ Belum Tersedia

| Fitur                            | Catatan                                |
| -------------------------------- | -------------------------------------- |
| For loop (C-style)               | Hanya `while`                          |
| Destructuring                    | Belum ada                              |
| Class / OOP                      | Hanya `struct`                         |
| Try/Catch                        | Error handling belum ada               |
| Generator / Iterator             | Belum ada                              |
| Decorator                        | Belum ada                              |

Catatan: kebutuhan conditional-expression sudah tertutup oleh fallback chain
`x = primary | fallback | default` (lihat `docs/syntax/fallback.md`) — Rupa
sengaja tidak meniru ternary `?:` atau arrow function dari bahasa lain.

---

## 2. Standard Library (C)

### ✅ Sudah Siap

| Module       | Import                       | Fungsi                                                                                               | Docs |
| ------------ | ---------------------------- | ---------------------------------------------------------------------------------------------------- | ---- |
| **os**       | `import os from rupa`        | `exec`, `getcwd`, `exit`, `getenv`, `chdir`, `mkdir`, `remove`, `rename`, `listdir`, `info`          | ✅   |
| **io**       | `import io from rupa`        | `input`, `toNumber`                                                                                  | ✅   |
| **json**     | `import json from rupa`      | `stringify`, `parse`, `valid`, `keys`, `values`, `merge`, `get`                                      | ✅   |
| **thread**   | `import thread from rupa`    | `create`, `join`, `sleep`, `id`, `count`                                                             | ✅   |
| **math** (C) | `import math from rupa`      | `abs`, `sqrt`, `pow`, `floor`, `ceil`, `round`, `sin`, `cos`, `tan`                                  | ✅   |
| **string**   | `import stdstring from rupa` | `length`, `upper`, `lower`, `trim`, `contains`, `startsWith`, `endsWith`, `replace`                  | ✅   |
| **http**     | `import http from rupa`      | `server`, `stop`, `request`, `get`, `post`, `put`, `delete`, `patch`                                 | ✅   |
| **datetime** | `import datetime from rupa`  | `now`, `nowMs`, `format`, `parse`, `diff`, `add`, `year`, `month`, `day`, `hour`, `minute`, `second` | ✅   |
| **regex**    | `import regex from rupa`     | `match`, `find`, `findAll`, `replace`, `split`                                                       | ✅   |
| **crypto**   | `import crypto from rupa`    | `md5`, `sha1`, `sha256`, `sha512`, `hmac`, `base64Encode`, `base64Decode`                            | ✅   |
| **net**      | `import net from rupa`       | `connect`, `send`, `receive`, `close`, `listen`, `accept`, `resolve`                                 | ✅   |

### ❌ Belum Tersedia

| Module         | Deskripsi                                         |
| -------------- | ------------------------------------------------- |
| **ui/view**    | UI rendering (terencana di `docs/syntax/view.md`) |

### 📦 Rupa Packages (stdlib)

| Module         | Bentuk import                  | Deskripsi                                                              | Status |
| -------------- | ------------------------------ | ---------------------------------------------------------------------- | ------ |
| **sys**        | `import sys from rupa`         | Informasi sistem + utilitas path (facade `os`)                          | ✅     |
| **fs**         | `import fs from rupa`          | Filesystem: read/write/lines/copy/move + path helpers (native `fsbase`) | ✅     |
| **database**   | `import db from rupa`          | Penyedia koneksi: nosql, mariadb, mysql, psql, sqlite (registry + DSN)  | ✅     |
| **collections**| `import collections from rupa` | Array/collection utilities                                              | ✅     |
| **strings**    | `import strings from rupa`     | String utilities                                                        | ✅     |

---

## 3. Module System

### ✅ Sudah Siap

| Fitur                    | Contoh                                    | Status |
| ------------------------ | ----------------------------------------- | ------ |
| Import single            | `import os from rupa`                     | ✅     |
| Import from local file   | `import db from ./database`               | ✅     |
| Import specific function | `import info from rupa.os`                | ✅     |
| Import with alias        | `import os as OS from rupa`               | ✅     |
| Export                   | `export fn from ./module`                 | ✅     |
| Export with alias        | `export db -> { getUser }`                | ✅     |
| Wildcard import          | `import http.*, thread.* from rupa as ns` | ✅     |
| Archive system           | `modules/rupa_modules.tar.gz`             | ✅     |

### 🔨 Dalam Pengembangan

| Fitur                  | Status | Catatan                                                                                                                |
| ---------------------- | ------ | ---------------------------------------------------------------------------------------------------------------------- |
| Multi-import from rupa | ✅     | `import http, thread, os from rupa` — grammar works                                                                    |
| Design NODE_MOD        | ✅     | Migrasi selesai: grammar, formatter, printer, interpreter, loader memakai satu container `NODE_MOD` untuk import/export |

> Standard Library = 11 modul native (C) + 5 Rupa packages (sys, fs,
> database, collections, strings). Documentation mengikuti tabel bagian 7.

### ❌ Belum Tersedia

| Fitur                       | Catatan                              |
| --------------------------- | ------------------------------------ |
| Package registry (npm-like) | `rupa install` ada tapi belum stabil |
| Version pinning             | Belum ada                            |
| Lock file                   | Belum ada                            |

---

## 4. Formatter & REPL & Editor

### ✅ Formatter (fmt) — Sudah Siap

| Fitur                        | Contoh                                               | Status |
| ---------------------------- | ---------------------------------------------------- | ------ |
| Format file                  | `rupa fmt file.rp`                                   | ✅     |
| Format stdin                 | `rupa fmt -` (untuk vim `:%!rupa fmt -`)             | ✅     |
| Spasi operator otomatis      | `x=1` → `x = 1`, `z?=true->x` → `z ?= true -> x`     | ✅     |
| Comment (`#`, `//`, `/* */`) | Dipertahankan & diratakan, `*/` closing diberi spasi | ✅     |
| Blank line preservation      | Baris kosong antar deklarasi dipertahankan           | ✅     |
| Import round-trip            | Semua 8 bentuk import dari docs identik & idempotent | ✅     |
| Self-heal import             | `a as form.*` rusak → pulih ke `a.* as form` kanonik | ✅     |
| Conditional assign           | `result ?= valid -> "Sukses"` dipertahankan          | ✅     |

Arsitektur formatter modular di `src/compiler/formatter/`:

| File                | Tanggung jawab                                                                       |
| ------------------- | ------------------------------------------------------------------------------------ |
| `formatter.c`       | Entry points (`formatFile`, `formatString`, `formatStdin`) + source-based formatting |
| `format_helpers.c`  | Shared utilities (`fmtIndent`, `fmtStr`, `fmtChar`, `fmtNewline`, `fmtSep`)          |
| `format_node.c`     | Atom nodes (identifier, literal, number, boolean, string, null)                      |
| `format_expr.c`     | Expressions (binary, call, print, array, object, member, dll)                        |
| `format_stmt.c`     | Statements (assign, if, loop, function, import/export, dll)                          |
| `format_dispatch.c` | Switch `fmtNode` utama                                                               |
| `format_comment.c`  | Deteksi & format comment                                                             |

### REPL & Editor

### ✅ Sudah Siap

| Fitur                               | Status |
| ----------------------------------- | ------ |
| Interactive REPL                    | ✅     |
| Line numbers                        | ✅     |
| Indentation (auto)                  | ✅     |
| Command `.help`, `.exit`, `.clear`  | ✅     |
| Multiline input (`{`, `(`, `[`)     | ✅     |
| Backspace rollback to previous line | ✅     |
| History                             | ✅     |

### 🔨 Dalam Pengembangan

| Fitur                 | Status | Catatan                         |
| --------------------- | ------ | ------------------------------- |
| Bracket pair matching | 🔨     | Nested `{}` indent tidak akurat |

---

## 5. Compiler (`-c`)

### ❌ Belum Tersedia

| Fitur                       | Status | Catatan                                   |
| --------------------------- | ------ | ----------------------------------------- |
| `rupa -c file.rp -o binary` | ❌     | Transpile to C → compile to native binary |

### Design: Transpile to C

```
rupa -c main.rp -o main

Internal pipeline:
1. Parse main.rp → AST
2. AST → C source code (transpiler)
3. gcc main.c -o main (with embedded runtime)
```

**Workflow:**

- User writes: `rupa -c main.rp -o main`
- Rupa transpiles AST to C source
- GCC compiles C to native binary (Linux/macOS)
- Output: self-contained binary, tidak perlu `rupa` runtime

**Pertimbangan:**

- Runtime (GC, stdlib, modules) di-embed ke binary
- Binary size: ~200-500KB (dengan runtime)
- Hanya untuk POSIX (Linux, macOS) — Windows butuh MinGW
- Async/thread perlu pthread di C output

**Status:** Design tersimpan, belum diimplementasi. Butuh transpiler AST → C yang cukup kompleks.

---

## 6. Testing

### ✅ Pass (45/45 — `./build.sh test`)

| Test                      | Deskripsi                    |
| ------------------------- | ---------------------------- |
| `annotation.rp`           | Type annotation              |
| `comment.rp`              | Comment (hash, slash, block) |
| `assignment.rp`           | Assignment                   |
| `async.rp`                | Async handler                |
| `function.rp`             | Function declaration         |
| `function-param-type.rp`  | Typed params                 |
| `keyword.rp`              | Keywords                     |
| `literal.rp`              | Literals                     |
| `loop.rp`                 | While loop                   |
| `module.rp`               | Module import/export         |
| `object.rp`               | Object                       |
| `object-in-array.rp`      | Object dalam array           |
| `print.rp`                | Print                        |
| `print_expr.rp`           | Print expression             |
| `print_fn.rp`             | Print function               |
| `string.rp`               | String                       |
| `struct.rp`               | Struct                       |
| `test_block_oneline.rp`   | Block scoping                |
| `test_block_scope.rp`     | Block scope                  |
| `test_block_scope2.rp`    | Block scope 2                |
| `test_block_simple.rp`    | Simple block                 |
| `test_case.rp`            | Case statement               |
| `test_case_return.rp`     | Case with return             |
| `test_func_min.rp`        | Minimal function             |
| `test_func_obj.rp`        | Function with object         |
| `test_func_step.rp`       | Function step                |
| `test_if.rp`              | If statement                 |
| `test_if_debug.rp`        | If debug                     |
| `test_interpreter.rp`     | Interpreter                  |
| `test_json_debug2.rp`     | JSON debug 2                 |
| `test_json_module.rp`     | JSON module                  |
| `test_member_assign.rp`   | Member assignment            |
| `test_multiline_block.rp` | Multiline block              |
| `test_rupa_root_math.rp`  | Math from rupa root          |
| `test_single_import.rp`   | Single import                |
| `test_stdlib.rp`          | Stdlib functions             |
| `test_this_member.rp`     | This member access           |
| `test_thread_async.rp`    | Thread + async               |
| `test_thread_async2.rp`   | Thread + async 2             |
| `test_thread_async3.rp`   | Thread + async 3             |
| `test_thread_async4.rp`   | Thread + async 4             |
| `test_thread_async5.rp`   | Thread + async 5             |
| `test_thread_async6.rp`   | Thread + async 6             |

### ❌ Fail (0)

Tidak ada test yang fail. Catatan cakupan:

- `tests/execution/` punya runner sendiri (`--test-exec`, 29 file) — di luar
  hitungan test suite syntax.
- Scan formatter `./build.sh fmt -` (110 file): 109 PASS; satu-satunya FAIL
  `tests/stress/index.rp` by-design (isi hanya `.` untuk memicu LexerError).

---

## 7. Documentation

### ✅ Sudah Siap

| Dokumen              | Path                               | Status |
| -------------------- | ---------------------------------- | ------ |
| Syntax Index         | `docs/syntax/index.md`             | ✅     |
| Grammar Index        | `docs/grammar/index.md`            | ✅     |
| Main Syntax          | `docs/syntax/main.md`              | ✅     |
| Import               | `docs/syntax/import.md`            | ✅     |
| Export               | `docs/syntax/export.md`            | ✅     |
| Module: Math         | `docs/modules/syntax/math.md`      | ✅     |
| Module: OS           | `docs/modules/syntax/os.md`        | ✅     |
| Module: IO           | `docs/modules/syntax/io.md`        | ✅     |
| Module: JSON         | `docs/modules/syntax/json.md`      | ✅     |
| Module: String       | `docs/modules/syntax/string.md`    | ✅     |
| Module: Thread       | `docs/modules/syntax/thread.md`    | ✅     |
| Module: HTTP         | `docs/modules/syntax/http.md`      | ✅     |
| Module: Math (G)     | `docs/modules/grammar/math.md`     | ✅     |
| Module: OS (G)       | `docs/modules/grammar/os.md`       | ✅     |
| Module: IO (G)       | `docs/modules/grammar/io.md`       | ✅     |
| Module: JSON (G)     | `docs/modules/grammar/json.md`     | ✅     |
| Module: String (G)   | `docs/modules/grammar/string.md`   | ✅     |
| Module: Thread (G)   | `docs/modules/grammar/thread.md`   | ✅     |
| Module: HTTP (G)     | `docs/modules/grammar/http.md`     | ✅     |
| Module: DateTime     | `docs/modules/syntax/datetime.md`  | ✅     |
| Module: DateTime (G) | `docs/modules/grammar/datetime.md` | ✅     |
| Module: Regex        | `docs/modules/syntax/regex.md`     | ✅     |
| Module: Regex (G)    | `docs/modules/grammar/regex.md`    | ✅     |
| Module: Crypto       | `docs/modules/syntax/crypto.md`    | ✅     |
| Module: Crypto (G)   | `docs/modules/grammar/crypto.md`   | ✅     |
| Module: Net          | `docs/modules/syntax/net.md`       | ✅     |
| Module: Net (G)      | `docs/modules/grammar/net.md`      | ✅     |
| Module: Sys          | `docs/modules/syntax/sys.md`       | ✅     |
| Module: Fs           | `docs/modules/syntax/fs.md`        | ✅     |
| Module: Database     | `docs/modules/syntax/database.md`  | ✅     |

### ❌ Belum Dibuat

| Dokumen                              | Catatan                     |
| ------------------------------------ | --------------------------- |
| `docs/modules/syntax/collections.md` | Collections (Rupa package)  |
| `docs/modules/syntax/strings.md`     | String utils (Rupa package) |

---

## 8. Pending Tasks (Prioritas)

### High Priority

1. ~~**Fix HTTP handler thread-safety**~~ ✅ Done — Queue-based approach: server thread handles network I/O, main thread calls handler via interpretNode
2. ~~**Fix failing tests**~~ — ✅ Done 13 tests masih fail (sebagian besar karena package `stark` tidak tersedia)
3. ~~**Add thread module docs**~~ ✅ Done — `docs/modules/syntax/thread.md` and `docs/modules/grammar/thread.md`

### Medium Priority

4. ~~**Multi-import `import a, b from rupa`**~~ ✅ Grammar works
5. ~~**Fix HTTP test**~~ — ✅ Done `test_http.rp` timeout karena server blocking
6. ~~**Add `#` comment support**~~ ✅ Done — `#`, `//`, `/* */` diparse jadi token, diabaikan interpreter
7. ~~**Fix array type checking**~~ — ✅ Done `x: number[] = [1, "a"]` currently raises TypeError (may be correct behavior)
8. ~~**Migrasi NODE_MOD**~~ — ✅ Done grammar emit `NODE_MOD` → formatter/printer `case NODE_MOD` (round-trip tabel pemetaan) → interpreter binding → hapus 4 node lama (`NODE_IMPORT`, `NODE_MODULE_IMPORT`, `NODE_EXPORT`, `NODE_EXPORT_DECL`)

### Low Priority

8. **Add for loop** — C-style `for (i=0; i<n; i++)`
9. **Try/Catch error handling** — Error handling yang proper
10. ~~**Database module**~~ — ✅ Done `stdlib/database`: registry driver (nosql/mariadb/mysql/psql/sqlite) + parser DSN + `db.open()` validasi; I/O jaringan menyusul
11. **UI/View module** — Rendering komponen UI
12. **`-c` Compilation** — Transpile to C → native binary (design tersimpan)
