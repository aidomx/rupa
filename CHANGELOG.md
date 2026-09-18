# Changelog

Format mengikuti [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased] — 2026-09-18

### Added

- **Class** — deklarasi class tanpa keyword:
  `Name: Type {}` diparse ke node `NODE_CLASS_DECL` baru (unit grammar
  `grammar_class.c`), bukan lagi menumpang struct. AST printer menampilkan
  `Class:` + `Type:`, formatter mempertahankan anotasi `: Type` (round-trip).
  Runtime: registrasi type + layout field sejajar struct (interpreter + IR).
  Nama constructor ditetapkan `construct()`.
- **Return-type annotation + void** — `foo(): void { }` / `foo(): number { }`:
  keyword `void`, grammar `AstFunctionDecl.returnType`, printer AST, formatter.
  Enforcement void di interpreter (declaration + call) dan IR
  (IR_RETURN + execCall closure path): fungsi void yang `return` nilai =
  `TypeError` fatal. Bare `return` tanpa nilai sah (kembalikan null).
- **Number 64-bit** — union `as.number` diperlebar int32 → long long:
  aritmetika int (interpreter + IR), literal AST, semua formatter/print `%lld`,
  stdlib yang menerima number ikut 64-bit. `sizeof(number)` = 8 (analyzer,
  rupamemory, provenance memory.c; blok 4-byte lama tetap didukung).
- **Test baru**: `tests/syntax/number64.rp`, `tests/syntax/void_ret.rp`,
  `tests/syntax/class.rp`.

### Fixed

- **Bare `return` di akhir blok** gagal lex (`LexerError: incomplete or
  invalid input`) — bug pre-existing sejak sebelum perubahan ini:
  - Lexer (`construct.c`): newline saat `expectValue` setelah `return`
    telanjang kini mengakhiri statement di file mode (guard `!bracket &&
    !paren` — `return {` multiline object tetap bekerja).
  - Grammar (`grammar_return.c`): `return` tanpa ekspresi sah — node RETURN
    dengan `expression = -1`, bukan error parse.
  - `run()` (file mode) kini `createGlobalState(length, false)` — sebelumnya
    `state->isRepl` bernilai true di file mode (sekalian print() tidak lagi
    menambah newline ekstra REPL).
- Formatter memangkas anotasi `: Type` pada class decl (kini round-trip).

### Changed

- **File mode jalan via IR** — `rupa <file.rp>` mengeksekusi lewat IR machine
  (`runWithIR = true`); interpreter tree-walking tetap tersedia via
  `--test` / `--test-exec`.
- Perkembangan interpreter dan IR wajib sinkron (keduanya jalur eksekusi
  default sekarang).
- `docs/TODO.md` diringkas: status ringkas per kategori, tanpa narasi panjang.

## [2026-09-16] — refactor arsitektur

- Restructure compiler, runtime, dan arsitektur proyek (commit 2026-09-16).
