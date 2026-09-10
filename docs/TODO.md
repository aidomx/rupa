# TODO — Rupa Language

> Dokumentasi status semua fitur: yang sudah siap, yang sedang dikerjakan, dan yang belum tersedia.
> Terakhir diperbarui: 10 September 2026

---

## Ringkasan Status

| Kategori | Siap | Dalam Pengembangan | Belum |
|----------|------|-------------------|-------|
| Syntax & Grammar | 24 | 3 | 5 |
| Standard Library | 7 | 1 | 5 |
| Module System | 4 | 1 | 2 |
| REPL & Editor | 3 | 1 | 0 |
| Testing | 42 | - | 13 |
| Documentation | 28 | 1 | 6 |

---

## 1. Syntax & Grammar

### ✅ Sudah Siap

| Fitur | Syntax Docs | Grammar Docs | Status |
|-------|-------------|--------------|--------|
| Literal | `docs/syntax/literal.md` | `docs/grammar/literal.md` | ✅ |
| Print | `docs/syntax/print.md` | `docs/grammar/print.md` | ✅ |
| Assignment | `docs/syntax/assignment.md` | `docs/grammar/assignment.md` | ✅ |
| Expression | `docs/syntax/expression.md` | `docs/grammar/expression.md` | ✅ |
| String | `docs/syntax/string.md` | `docs/grammar/string.md` | ✅ |
| Array | `docs/syntax/array.md` | `docs/grammar/array.md` | ✅ |
| Object | `docs/syntax/object.md` | `docs/grammar/object.md` | ✅ |
| If/Else | `docs/syntax/if.md` | `docs/grammar/if.md` | ✅ |
| Block | `docs/syntax/block.md` | `docs/grammar/block.md` | ✅ |
| Function | `docs/syntax/function.md` | `docs/grammar/function.md` | ✅ |
| Return | `docs/syntax/return.md` | `docs/grammar/return.md` | ✅ |
| Loop (while) | `docs/syntax/loop.md` | `docs/grammar/loop.md` | ✅ |
| Case | `docs/syntax/case.md` | `docs/grammar/case.md` | ✅ |
| Call | `docs/syntax/call.md` | `docs/grammar/call.md` | ✅ |
| Struct | `docs/syntax/struct.md` | `docs/grammar/struct.md` | ✅ |
| Annotation | `docs/syntax/annotation.md` | `docs/grammar/annotation.md` | ✅ |
| Update/Increment | `docs/syntax/update.md` | `docs/grammar/update.md` | ✅ |
| Module Import | `docs/syntax/import.md` | `docs/grammar/import.md` | ✅ |
| Module Export | `docs/syntax/export.md` | `docs/grammar/export.md` | ✅ |
| Control Flow | `docs/syntax/control.md` | `docs/grammar/control.md` | ✅ |
| Fallback | `docs/syntax/fallback.md` | `docs/grammar/fallback.md` | ✅ |
| Then (inline) | `docs/syntax/then.md` | `docs/grammar/then.md` | ✅ |
| Annotation | `docs/syntax/annotation.md` | `docs/grammar/annotation.md` | ✅ |
| Member Access | `docs/syntax/struct.md` | `docs/grammar/member.md` | ✅ |

### 🔨 Dalam Pengembangan

| Fitur | Syntax Docs | Grammar Docs | Status | Catatan |
|-------|-------------|--------------|--------|---------|
| Async/Await | `docs/syntax/async.md` | `docs/grammar/async.md` | 🔨 | `await` syntax & handler blok belum stabil |
| HTTP | `docs/modules/syntax/http.md` | `docs/modules/grammar/http.md` | ✅ | Handler user-defined works via queue-based thread safety |
| Comment | - | - | 🔨 | `//` dan `/* */` bekerja, `#` tidak didukung |

### ❌ Belum Tersedia

| Fitur | Catatan |
|-------|---------|
| Ternary / Conditional Expression | Belum ada |
| For loop (C-style) | Hanya `while` |
| Destructuring | Belum ada |
| Class / OOP | Hanya `struct` |
| Try/Catch | Error handling belum ada |
| Switch statement | Menggunakan `case` (different syntax) |
| Arrow function | `=>` sudah dipakai untuk async handler |
| Generator / Iterator | Belum ada |
| Decorator | Belum ada |

---

## 2. Standard Library (C)

### ✅ Sudah Siap

| Module | Import | Fungsi | Docs |
|--------|--------|--------|------|
| **os** | `import os from rupa` | `exec`, `getcwd`, `exit`, `getenv`, `chdir`, `mkdir`, `remove`, `rename`, `listdir`, `info` | ✅ |
| **io** | `import io from rupa` | `input`, `toNumber` | ✅ |
| **json** | `import json from rupa` | `stringify`, `parse`, `valid`, `keys`, `values`, `merge`, `get` | ✅ |
| **thread** | `import thread from rupa` | `create`, `join`, `sleep`, `id`, `count` | ✅ |
| **math** (C) | `import math from rupa` | `abs`, `sqrt`, `pow`, `floor`, `ceil`, `round`, `sin`, `cos`, `tan` | ✅ |
| **string** | `import stdstring from rupa` | `length`, `upper`, `lower`, `trim`, `contains`, `startsWith`, `endsWith`, `replace` | ✅ |
| **http** | `import http from rupa` | `server`, `stop`, `request`, `get`, `post`, `put`, `delete`, `patch` | ✅ |

### 🔨 Dalam Pengembangan

| Module | Status | Catatan |
|--------|--------|---------|
| ~~**http**~~ | ✅ | Queue-based handler thread-safety fixed. User-defined handler dipanggil dari main thread. Request/Response object tersedia. `res.setHeader()`, `res.json()` works. |

### ❌ Belum Tersedia

| Module | Deskripsi |
|--------|-----------|
| **crypto** | Hash, encrypt, decrypt |
| **net** | Socket, TCP/UDP |
| **regex** | Regular expression |
| **datetime** | Date/time manipulation |
| **database** | SQL/NoSQL client |
| **filesystem** | File read/write (beyond os) |
| **ui/view** | UI rendering (terencana di `docs/syntax/view.md`) |

---

## 3. Module System

### ✅ Sudah Siap

| Fitur | Contoh | Status |
|-------|--------|--------|
| Import single | `import os from rupa` | ✅ |
| Import from local file | `import db from ./database` | ✅ |
| Import specific function | `import info from rupa.os` | ✅ |
| Import with alias | `import os as OS from rupa` | ✅ |
| Export | `export fn from ./module` | ✅ |
| Export with alias | `export db -> { getUser }` | ✅ |
| Wildcard import | `import http.*, thread.* from rupa as ns` | ✅ |
| Archive system | `modules/rupa_modules.tar.gz` | ✅ |

### 🔨 Dalam Pengembangan

| Fitur | Status | Catatan |
|-------|--------|---------|
| Multi-import from rupa | ✅ | `import http, thread, os from rupa` — grammar works |

### ❌ Belum Tersedia

| Fitur | Catatan |
|-------|---------|
| Package registry (npm-like) | `rupa install` ada tapi belum stabil |
| Version pinning | Belum ada |
| Lock file | Belum ada |

---

## 4. REPL & Editor

### ✅ Sudah Siap

| Fitur | Status |
|-------|--------|
| Interactive REPL | ✅ |
| Line numbers | ✅ |
| Indentation (auto) | ✅ |
| Command `.help`, `.exit`, `.clear` | ✅ |
| Multiline input (`{`, `(`, `[`) | ✅ |
| Backspace rollback to previous line | ✅ |
| History | ✅ |

### 🔨 Dalam Pengembangan

| Fitur | Status | Catatan |
|-------|--------|---------|
| Bracket pair matching | 🔨 | Nested `{}` indent tidak akurat |

---

## 5. Testing

### ✅ Pass (42)

| Test | Deskripsi |
|------|-----------|
| `annotation.rp` | Type annotation |
| `assignment.rp` | Assignment |
| `async.rp` | Async handler |
| `function.rp` | Function declaration |
| `function-param-type.rp` | Typed params |
| `keyword.rp` | Keywords |
| `literal.rp` | Literals |
| `loop.rp` | While loop |
| `module.rp` | Module import/export |
| `object.rp` | Object |
| `object-in-array.rp` | Object dalam array |
| `print.rp` | Print |
| `print_expr.rp` | Print expression |
| `print_fn.rp` | Print function |
| `string.rp` | String |
| `struct.rp` | Struct |
| `test_block_oneline.rp` | Block scoping |
| `test_block_scope.rp` | Block scope |
| `test_block_scope2.rp` | Block scope 2 |
| `test_block_simple.rp` | Simple block |
| `test_case.rp` | Case statement |
| `test_case_return.rp` | Case with return |
| `test_func_min.rp` | Minimal function |
| `test_func_obj.rp` | Function with object |
| `test_func_step.rp` | Function step |
| `test_if.rp` | If statement |
| `test_if_debug.rp` | If debug |
| `test_interpreter.rp` | Interpreter |
| `test_json_debug2.rp` | JSON debug 2 |
| `test_json_module.rp` | JSON module |
| `test_member_assign.rp` | Member assignment |
| `test_multiline_block.rp` | Multiline block |
| `test_rupa_root_math.rp` | Math from rupa root |
| `test_single_import.rp` | Single import |
| `test_stdlib.rp` | Stdlib functions |
| `test_this_member.rp` | This member access |
| `test_thread_async.rp` | Thread + async |
| `test_thread_async2.rp` | Thread + async 2 |
| `test_thread_async3.rp` | Thread + async 3 |
| `test_thread_async4.rp` | Thread + async 4 |
| `test_thread_async5.rp` | Thread + async 5 |
| `test_thread_async6.rp` | Thread + async 6 |

### ❌ Fail (13)

| Test | Error | Kemungkinan Penyebab |
|------|-------|---------------------|
| `array.rp` | Type mismatch | `TypeError` raised (expected: `[1, "Hello"]` has string in number[] — correct behavior) |
| `case.rp` | Scope error | Variable scope dalam case |
| `colon-context.rp` | - | Type annotation syntax |
| `comment.rp` | - | `#` comment tidak didukung |
| `expression.rp` | - | Expression tertentu belum didukung |
| `test_http.rp` | Timeout (124) | Server test needs timeout adjustment |
| `test_json_debug.rp` | - | Module resolution |
| `test_local_archive.rp` | - | Archive package tidak ditemukan |
| `test_local_import.rp` | - | `stark` package tidak ditemukan |
| `test_minimal.rp` | - | `stark` package tidak ditemukan |
| `test_multi_import.rp` | - | Multi-import belum stabil |
| `test_rupa_stark.rp` | - | `rupa.stark` tidak tersedia |
| `test_stark_add.rp` | - | `stark` package tidak tersedia |

---

## 6. Documentation

### ✅ Sudah Siap

| Dokumen | Path | Status |
|---------|------|--------|
| Syntax Index | `docs/syntax/index.md` | ✅ |
| Grammar Index | `docs/grammar/index.md` | ✅ |
| Main Syntax | `docs/syntax/main.md` | ✅ |
| Import | `docs/syntax/import.md` | ✅ |
| Export | `docs/syntax/export.md` | ✅ |
| Module: Math | `docs/modules/syntax/math.md` | ✅ |
| Module: OS | `docs/modules/syntax/os.md` | ✅ |
| Module: IO | `docs/modules/syntax/io.md` | ✅ |
| Module: JSON | `docs/modules/syntax/json.md` | ✅ |
| Module: String | `docs/modules/syntax/string.md` | ✅ |
| Module: Thread | `docs/modules/syntax/thread.md` | ✅ |
| Module: HTTP | `docs/modules/syntax/http.md` | ✅ |
| Module: Math (G) | `docs/modules/grammar/math.md` | ✅ |
| Module: OS (G) | `docs/modules/grammar/os.md` | ✅ |
| Module: IO (G) | `docs/modules/grammar/io.md` | ✅ |
| Module: JSON (G) | `docs/modules/grammar/json.md` | ✅ |
| Module: String (G) | `docs/modules/grammar/string.md` | ✅ |
| Module: Thread (G) | `docs/modules/grammar/thread.md` | ✅ |
| Module: HTTP (G) | `docs/modules/grammar/http.md` | ✅ |

### ❌ Belum Dibuat

| Dokumen | Catatan |
|---------|---------|
| `docs/modules/syntax/collections.md` | Collections (Rupa package) |
| `docs/modules/syntax/strings.md` | String utils (Rupa package) |

---

## 7. Pending Tasks (Prioritas)

### High Priority

1. ~~**Fix HTTP handler thread-safety**~~ ✅ Done — Queue-based approach: server thread handles network I/O, main thread calls handler via interpretNode
2. **Fix failing tests** — 13 tests masih fail (sebagian besar karena package `stark` tidak tersedia)
3. ~~**Add thread module docs**~~ ✅ Done — `docs/modules/syntax/thread.md` and `docs/modules/grammar/thread.md`

### Medium Priority

4. ~~**Multi-import `import a, b from rupa`**~~ ✅ Grammar works
5. **Fix HTTP test** — `test_http.rp` timeout karena server blocking
6. **Add `#` comment support** — Atau update docs bahwa tidak didukung
7. **Fix array type checking** — `x: number[] = [1, "a"]` currently raises TypeError (may be correct behavior)

### Low Priority

8. **Add for loop** — C-style `for (i=0; i<n; i++)`
9. **Try/Catch error handling** — Error handling yang proper
10. **Regex module** — Regular expression
11. **Crypto module** — Hash, encrypt
12. **Database module** — SQL/NoSQL client
13. **UI/View module** — Rendering komponen UI
