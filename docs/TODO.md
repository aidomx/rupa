# TODO — Rupa Language

> Status fitur: siap, dalam pengembangan, belum ada.
> Terakhir diperbarui: 18 September 2026

---

## Ringkasan

| Kategori          | Siap | Dalam Pengembangan | Belum |
| ----------------- | ---- | ------------------ | ----- |
| Syntax & Grammar  | 27   | 1                  | 5     |
| Standard Library  | 16   | 0                  | 1     |
| Module System     | 11   | 0                  | 3     |
| REPL & Editor     | 14   | 1                  | 0     |
| Compiler (`-c`)   | 0    | 1                  | 1     |
| Testing           | 49   | -                  | 0     |
| Documentation     | 30   | 0                  | 4     |

Testing: syntax **49/49** (`--test`), IR rewrite **49/49** (`--test-ir`),
IR exec **49/49** (`--test-irexec`), execution **19/19** (`--test-exec` atas
13 file `tests/execution/` + 6 `tests/semantics/`), REPL **18/18**
(`--test-repl`). Formatter: 113 file di-scan, 1 FAIL by-design
(`tests/stress/index.rp` memang memicu LexerError).

---

## 1. Syntax & Grammar

### ✅ Siap

Literal, Print, Assignment, Expression, String, Array, Object, If/Else,
Block, Function, Return, Loop (while), Case, Call, Struct, Annotation,
Update/Increment (compound `+=` dst.), Import, Export, Control Flow,
Fallback, Then, Member Access, Comment — docs masing-masing di
`docs/syntax/*.md` + `docs/grammar/*.md`.

| Fitur baru        | Catatan                                                                 |
| ----------------- | ----------------------------------------------------------------------- |
| Number 64-bit     | `as.number` = long long; `sizeof(number)` = 8; print `%lld`             |
| Return-type + void | `foo(): void {}` — enforcement di interpreter & IR                     |
| Class             | `Name: Type {}` → `NODE_CLASS_DECL`; AST `Class:`, formatter round-trip |

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

Di luar hitungan: rupamemory (sizeof, pin/elpin/repin/repins) built-in global
tanpa import.

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
| syntax        | 49     | PASS   |
| ir            | 49     | PASS   |
| irexec        | 49     | PASS   |
| exec          | 19     | PASS   |
| repl          | 18     | PASS   |

Suite baru: `number64.rp` (overflow 32-bit & presisi 2^53), `void_ret.rp`
(return-type + void + bare return), `class.rp` (class decl + construct).

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
- Const/def/ifdef family — DITUNDA: scope-nya sub-sistem preprocessor
  C-style (const enum block, `def`, `ifdef`), bukan modifier binding
- Design tipe: number 64-bit ✅, void ✅, const ditunda, char/int skip,
  bigint defer
- new Contract / new T() / del / string slot — ✅ Done; terbuka:
  `new string()`, interaksi const × del
- Formatter: sunset docs pin family, rename block ops prefix `pin`

### Low

- For loop (C-style)
- Try/Catch error handling
- UI/View module
- `-c` compilation (IR → C)
