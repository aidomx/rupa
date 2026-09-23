# TODO — Rupa Language

> Status fitur: siap, dalam pengembangan, belum ada.
> Terakhir diperbarui: 21 September 2026

---

## Ringkasan

| Kategori          | Siap | Dalam Pengembangan | Belum |
| ----------------- | ---- | ------------------ | ----- |
| Syntax & Grammar  | 28   | 1                  | 5     |
| Standard Library  | 16   | 0                  | 1     |
| Module System     | 11   | 0                  | 3     |
| REPL & Editor     | 14   | 1                  | 0     |
| Compiler (`-c`)   | 0    | 1                  | 1     |
| Testing           | 54   | -                  | 0     |
| Documentation     | 32   | 0                  | 4     |

Testing: syntax **54/54** (`--test`), IR rewrite **54/54** (`--test-ir`),
IR exec **54/54** (`--test-irexec`), execution **19/19** (`--test-exec` atas
13 file `tests/execution/` + 6 `tests/semantics/`), REPL **31/31**
(`--test-repl`). Formatter: 113 file di-scan, 1 FAIL by-design
(`tests/stress/index.rp` memang memicu LexerError).

Batch runner baru: `rupa test [kategori] [--list] [--select n]` dengan
kategori `syntax` (default), `ast`, `ir`, `irexec`, `exec`, `semantics`,
`repl`, `fmt` — plus shortcut `-t`, `-l`, `-p`, `-s`
(mis. `rupa -ts ast 1`, `rupa -lp exec`). Rincian: `rupa help test`.

---

## 1. Syntax & Grammar

### ✅ Siap

Literal, Print, Assignment, Expression, String, Array, Object, If/Else,
Block, Function, Return, Loop (while), Case, Call, Struct, Enum, Annotation,
Update/Increment (compound `+=` dst.), Import, Export, Control Flow,
Fallback, Then, Member Access, Comment — docs masing-masing di
`docs/syntax/*.md` + `docs/grammar/*.md`.

| Fitur baru        | Catatan                                                                 |
| ----------------- | ----------------------------------------------------------------------- |
| Number 64-bit     | `as.number` = long long; `sizeof(number)` = 8; print `%lld`             |
| Return-type + void | `foo(): void {}` — enforcement di interpreter & IR                     |
| Class             | `Name: Type {}` → `NODE_CLASS_DECL`; AST `Class:`, formatter round-trip |
| Enum              | `enum Nama { MEMBER = 1 }` → AST member eksplisit; auto-increment       |

### 🔨 Dalam Pengembangan

| Fitur       | Catatan                                    |
| ----------- | ------------------------------------------ |
| Async/Await | `await` syntax & handler blok belum stabil |
| Class (lanjutan) | `this`/closure, `super`, `@created`, `@input` |

### ❌ Belum Tersedia

For loop (C-style), Destructuring, Try/Catch, Generator/Iterator, Decorator.

Catatan: conditional-expression tertutup oleh fallback chain
`x = primary | fallback` (lihat `docs/syntax/fallback.md`).

---

## 2. Standard Library (C)

### ✅ Siap — 11 modul native

os, io, json, thread, math, string (`stdstring`), http, datetime, regex,
crypto, net — docs di `docs/modules/syntax/*.md` + `docs/modules/grammar/*.md`.

### ✅ Siap — 5 Rupa packages

sys, fs, database, collections, strings.

Di luar hitungan: rupamemory (sizeof, blok ops contract ccpy/cmove/cset,
dup family) built-in global tanpa import. pin/elpin/repin/repins/unpin
**dihapus** — alokasi satu pintu lewat `new/del` + `new Contract()`;
blok ops lama `setpin/movepin/copypin` di-rename `cset/cmove/ccpy`.

### ❌ Belum Tersedia

ui/view (UI rendering).

---

## 3. Module System

### ✅ Siap

Import single/local/specific/alias/wildcard, Export + alias, Namespace export
block, Dotted-path export, Duplicate-export detection, Archive
(`modules/rupa_modules.tar.gz`).

### ❌ Belum Tersedia

Package registry (npm-like), Version pinning, Lock file.

---

## 4. Formatter & REPL & Editor

### ✅ Formatter

Format file/stdin, spasi operator, comment, blank line, import round-trip,
self-heal import, conditional assign. Arsitektur modular di `src/formatter/`.

### ✅ REPL & Editor

Interactive REPL, line numbers, indentation, command, multiline input,
backspace rollback, history.

### 🔨 Dalam Pengembangan

Bracket pair matching (nested `{}` indent tidak akurat —
`tests/execution/repl_struct.rp`).

---

## 5. Compiler (`-c`)

### 🔨 Dalam Pengembangan

IR pipeline (`src/compiler/ir/`): AST → IR (`rewrite.c`) + executor
(`execute.c`). **`rupa <file.rp>` kini jalan via IR** (`runWithIR = true` di
`src/prompt/runner.c`); interpreter tetap ada sebagai fallback
(`--test-exec`/`--test`). IR → C source (transpiler) belum diimplementasi.

### ❌ Belum Tersedia

`rupa -c file.rp -o binary` (transpile to C → native binary).

---

## 6. Testing

| Suite         | Jumlah | Status |
| ------------- | ------ | ------ |
| syntax        | 55     | PASS   |
| ir            | 55     | PASS   |
| irexec        | 55     | PASS   |
| exec          | 19     | PASS   |
| repl          | 18     | PASS   |
| fmt           | 1      | PASS   |

Suite baru: `number64.rp` (overflow 32-bit & presisi 2^53), `void_ret.rp`
(return-type + void + bare return), `class.rp` (class decl + construct),
`const.rp` (const + re-init loop/fungsi).

Dijalankan lewat batch runner: `rupa test`, `rupa test --list`,
`rupa test --select 1`, atau shortcut `rupa -t`, `rupa -l`, `rupa -lp ast`.

---

## 7. Documentation

### ✅ Siap

Index (syntax/grammar/modules), Main, Import, Export + docs module:
math, os, io, json, string, thread, http, datetime, regex, crypto, net,
sys, fs, database (syntax + grammar masing-masing).

### ❌ Belum Dibuat

`docs/modules/syntax/collections.md`, `docs/modules/syntax/strings.md`,
`docs/syntax/namespace.md`, `docs/grammar/namespace.md`.

---

## 8. Pending Tasks

### High

- (kosong)

### Medium

- Class lanjutan: `this`/closure → `super` + `@created` → runner `rupa go` +
  `@input`
- Const binding ✅ (`const x = v` slot immutable, ConstError saat
  reassign); def/ifdef family — DITUNDA: scope-nya sub-sistem
  preprocessor C-style (const enum block, `def`, `ifdef`)
- Design tipe: number 64-bit ✅, void ✅, const binding ✅, char/int skip,
  bigint defer
- new Contract / new T() / del / string slot — ✅ Done; terbuka:
  `new string()`, interaksi const × del
- pin family sunset ✅ — dihapus dari runtime; docs diarsipkan ke
  `docs/legacy/pin-family.md`, view check handle kini registry v3
  (`memoryHandleTypeCheck`)
- struct C-style ✅ — padding/alignment (sizeof sesuai ABI), nested
  struct by value + array of struct via view handle, `del` view
ditolak (test: `tests/execution/struct_layout.rp`)
- Formatter: rename block ops prefix `pin` — ✅ Done (`cset/cmove/ccpy`)

### Low

- For loop (C-style)
- Try/Catch error handling
- UI/View module
- `-c` compilation (IR → C)
