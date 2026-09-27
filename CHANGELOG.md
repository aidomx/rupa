# Changelog

Format mengikuti [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased] — 2026-09-27

### Added

- **Print format engine** — dua pola baru di
  atas pola 1 (interpolasi) & pola 2 (multi-arg) yang sudah ada:
  - **Pola 3 — printf-style**: `print("%s\n", "hello")`. Konversi
    `%s %d %i %u %x %X %o %f %F %e %E %g %G %c` + `%%`, dengan flags
    `-` `+` `0`, width, dan precision. `%s` me-stringify semua tipe
    sejajar pola 2; string tanpa konversi valid dicetak apa adanya
    (`"50%"` aman). Arg lebih banyak dari konversi disambung spasi.
  - **Pola 4 — stream target**: `print(stderr, format, ...)` — arg
    pertama `stdout`/`stderr` menjadi stream tujuan (format pola
    1/2/3 tetap berlaku di sisa arg). Binding baru `stdout`/`stderr`
    (`VALUE_PTR` handle `FILE*`, registered di `builtinsInit`),
    divalidasi ketat di `printStreamOf()` — handle memori (pin/
    Contract) tidak mungkin lolos.
  - Engine terpusat di `src/stdlib/print_format.c`
    (`printRenderArgs()`): satu pintu render untuk interpreter
    (statement.c), mesin IR (execute.c), dan REPL — file mode (IR)
    dan interpreter kini identik urutan & formatnya.
  - `value.c` kini menyediakan render-to-buffer (`valuePrintTo`,
    `printBufAppend`, `printStringInterpTo`) sehingga interpolasi
    pola 1 bisa diarahkan ke stream mana pun (pola 4).
  - Parser: literal `true`/`false`/`null` di dalam `{ }` kini bentuk
    node boolean/nullable langsung — `print("{true}")` menghasilkan
    `true`, bukan `null` (sejajar parseAtom + fallback evalInterpExpr
    untuk expression lain di jalur unresolved).
  - IR `buildPrint`: satu call `print` dengan SEMUA args (sebelumnya
    satu call per arg) agar engine melihat daftar arg utuh.
- **Test baru**: `tests/execution/print_format_stream.rp`.
- **`#` non-hex kini LexerError** — token HASHTAG tidak pernah sah di
  rupa (komentar = `//` atau `/* */`; `#` hanya hex color literal).
  Lexer menolak langsung (`x = 1 # zz`, `1 + # 2`, baris `# ...`)
  — sebelumnya token ditelan parser diam-diam bila statement lain
  sukses. Error muncul konsisten di semua jalur: file, `-e`, REPL.
- **Error lexer/parser tidak dibuang diam lagi**:
  - `runner.c` (file mode): SyntaxError parse = fatal meski statement
    lain sukses — dulu error dibuang dan program jalan parsial
    (baris `# ...` di antara statement lolos tanpa jejak).
  - `generateAst` (ast.c, jalur `-e`/REPL): Request kini membawa
    Error (dulu NULL) — SyntaxError parse tercetak dan eksekusi
    dibatalkan; error runtime tetap dicetak sekali oleh interpreter.
  - `processInput` (input.c): LexerError dicetak + reset size
    (pola repl_input.c) — dulu ditelan, `-e` exit 0 sukses.
- `grammar_comment.c`: cabang HASHTAG+COMMENT dihapus (mati sejak
  `#` menjadi hex color literal).
- **`rupa profile` silent + Cache akurat** — dua perbaikan:
  - **Output program di-suppress**: stdout dialihkan ke /dev/null
    (NUL di Windows) selama eksekusi terukur; yang tampil hanya
    laporan profiler. Error runtime/lexer tetap terlihat (stderr).
  - **Cache: selalu miss → warmup run**: cache pipeline (AST+IR)
    bersifat in-process — satu file dieksekusi tepat sekali per
    proses, jadi mustahil hit. `profileRun` kini dua fase: warmup
    senyap tanpa profiler (mengisi cache) lalu measured run
    (`Cache: hit`, lexer/parser/rewrite = 0.0 ms karena ter-cache);
    pengukuran tidak tercemar stage warmup. Error warmup tidak
    dobel (stderr ikut disuppress khusus warmup).
- **Color print**
  - `#` dilepas dari peran komentar → hex color literal `#rgb` /
    `#rrggbb` (ala CSS, 3-digit di-expand) berupa NUMBER 24-bit.
    Komentar satu baris kini `//`, blok `/* */`; `#` non-hex =
    HASHTAG → SyntaxError.
  - Lexer `0x...`: literal hex biasa (sebelumnya LexerError).
  - Binding warna bawaan `U_RED/U_GREEN/.../U_CYAN` (VALUE_COLOR);
    `print(U_RED, "%s", ...)` merender ANSI truecolor
    `\033[38;2;R;G;Bm ... \033[0m` di print engine (interpreter + IR).
  - Custom warna: `enum Color { BLACK: color = #000000 }` — member
    enum bertipe `color` berjalan sebagai stream/color target pola 4.
  - `valueString`: escape `\xNN` (1–2 digit hex → raw byte) untuk
    ANSI manual; `processor.c rollback()` fix double-free token GC.
- **Enforcement return-type non-void** — `getName(): string { return 1 }`
  kini `TypeError: function 'getName' declared to return 'string' but
got 'number'` (sebelumnya hanya `void` yang di-enforce). Validasi
  scalar, struct terdaftar, `T[]` (nested), dan VALUE_PTR handle
  Contract (`memoryHandleTypeCheck`), dijalankan saat return/call:
  - IR: `buildReturn` emit `IR_CHECK` varian `irCheckFunctionAt`
    (payload nama fungsi) pada return eksplisit — lokasi error tetap
    node value (`baris:kolom` presisi); closure path `execCall`
    memvalidasi hasil FLOW_RETURN via `analyzerCheckReturnType` —
    nilai yang gagal kontrak tidak dipropagasi.
  - Interpreter: `interpretCall` memvalidasi hasil call (sejajar IR);
    return sintetis fall-through body tidak dicek (null, konsisten
    kedua jalur).
  - Pesan void diberi nama fungsi di semua titik:
    `function 'foo' is void and cannot return a value`
    (declaration probe, interpretCall, IR_RETURN, execCall closure).

### Changed

- Pola 2 kini merender seluruh arg ke satu buffer lalu ditulis
  sekali ke stream — output multi-arg tidak lagi terselang-seling
  dengan stderr.

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
- **Enum** — keyword `enum` aktif + bentuk AST eksplisit (member sebagai
  `Annotation(Name, Type, Value)` / `Identifier` bare, bukan statement
  acak). Akses member tak dikenal pada enum = `EnumError` (bukan null);
  penulisan member enum ditolak. Object biasa tetap backward compatible.
- **Const** — keyword `const` (lexer, sejajar bahasa lain): binding
  immutable `const x: number = 1` / `const y = x + 2`. Implementasi
  binding-level, bukan nilai: store deklarasi (IR `irStoreAt` ber-flag)
  menulis + mengunci slot; executor menolak store berikutnya ke slot
  terkunci (`ConstError`) saat eksekusi mencapai store — statement
  sebelum tetap jalan. Deklarasi const di loop/fungsi re-init sah.
- **Test baru**: `tests/syntax/number64.rp`, `tests/syntax/void_ret.rp`,
  `tests/syntax/class.rp`, `tests/syntax/enum.rp`, `tests/syntax/const.rp`.

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
