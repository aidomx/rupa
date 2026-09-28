# Changelog

Format mengikuti [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased] — 2026-09-28

### Added

- **`rupa profile --optimizer [root]`** — deteksi pekerjaan duplikasi
  lintas file: pindai file .rp (rekursif, default cwd), jalankan
  lex+parse per file dengan trace event pool canonical (publish/adopt
  di caches/syntax/canonical.c), lalu laporkan grup ekspresi yang
  di-adopt (>= 2 kemunculan) dengan file:baris pertama/terakhir,
  cuplikan sumber, dan saran patch (ekstraksi helper). Tidak mengubah
  file — patch otomatis sumber user berisiko; laporan bersifat saran.
- **`rupa profile --cache`** — laporan daftar cache in-process:
  pipeline (AST+IR per file, status valid/basi), canonical pool,
  statistik hit/miss/rate pipeline + canonical + memo parse, FileCache,
  dan module cache. API inspeksi baru: `pipelineCacheEntryCount/Path/
  Valid`, `syntaxCanonicalCount`, `moduleCacheCount`.
- **`rupa profile --clean`** — reset seluruh cache in-process
  (pipeline, canonical, FileCache, module) via `runnerFileCacheReset`
  + reset yang sudah ada. Berguna sebagai titik audit; cache CLI tunggal
  memang hidup hanya selama proses.
- **Header publik terverifikasi (item header publik librupa)** —
  audit konsumer eksternal: `#include <rupa.h>` + link `lib/librupa.a`
  / `lib/librupa.so` cukup untuk embed runtime penuh (loader/execute,
  semantic/value API, IR + debugIRModule, GC). Ditemukan & dibereskan:
  `lib/librupa.a` lama memuat objek basi `memory.o` (unit monolitik
  sebelum split) yang membuat link gagal simbol duplikat — arsip
  dibangun ulang bersih. Test konsumer: embed loader (`main` →
  `loader(argv, argc)`) menjalankan file .rp dan `-e` dengan benar.
- **`pub` — visibility eksplisit + namespace file-level (design fn &
  namespace)** — `pub a() {}` / `pub x = 10` menandai binding publik;
  default TANPA `pub` = private. Export surface module: bila file
  punya `pub`, hanya member pub yang diterbitkan (private = null +
  metadata `_private`, akses dari luar = PrivateError); file tanpa
  `pub` = perilaku lama, kompatibel penuh. Policy export lama tetap
  berlaku. Namespace file-level `namespace user` (tanpa `{ }`):
  seluruh export surface file dibungkus object bernama namespace —
  `import user from ./b` lalu `user.x`, `user.add(1,2)`; akses member
  private = PrivateError di jalur IR (IR_MEMBER_GET) dan interpreter
  (interpretMember). Implementasi: keyword `pub` (lexer/keyword), flag
  `isPub` (AstFunctionDecl/AstAssignment), `isPub` + `bare` di AstMod,
  binding flag `isPub` + env `hasPub` (semMarkPub/semHasPub), opcode
  `IR_MARK_PUB`, parser `namespace <nama>` bare di grammar_module_export,
  filter pub di buildWholeEnvValue, pembungkus namespace di loader.c.
- **Binding variabel loop implisit (design loop)** — `for i < 10 {}`
  tanpa deklarasi: `i` tidak lagi bocor ke scope luar; `print(i)`
  setelah loop = undefined (ReferenceError). Variable yang sudah ada
  (`i = 0; for i < 10`) tetap hidup dengan nilai akhirnya (10).
  Implementasi lintas jalur: `semUnsetLocal` + flag `dead` di binding
  (semGet/semFind melewati dead, semSet menghidupkan kembali), opcode
  IR baru `IR_PROBE_ABSENT`/`IR_UNBIND` (emit di buildLoop, probe env
  runtime sebagai sumber kebenaran), dan unbind di interpreter
  loop_for/loop_rev.
- **Statement satu baris `x = 1; y = 2` / `x = 1, y = 2`**
  (design/variable.txt) — `,`/`;` top-level memisah statement dalam
  satu baris via `grammarStatementSplit` di grammar.c; parser sebelumnya
  menelan separator dan menghasilkan AST salah (`x = 1 = 2`). `,` di
  akhir baris diperlakukan terminator kosong; statement ber-keyword
  tidak di-split (header `for i=0; i<10` dan daftar `import a, b`
  dikonsumsi grammar masing-masing). `grammarParseStatement` dipecah:
  `grammarParseStatementBody` menampung pemilihan grammar.
- **Backend C menutup gap opcode loop/pub** — `rupa <file> -o` gagal
  `unsupported IR opcode 34` untuk program ber-loop (`tests/stress/
  el.rp`): `IR_PROBE_ABSENT`/`IR_UNBIND`/`IR_MARK_PUB` (opcode baru
  loop binding + pub) kini di-emit ke C via helper `rupa_absent`/
  `rupa_unbind` + `semMarkPub`. Semantik compiled = interpreter
  (loop var implisit undefined setelah loop, pub surface).
- **Compile `-o` dari direktori mana pun** — codegen sebelumnya memakai
  path relatif (`-Iinclude -I.`, `lib/librupa.a`) yang menunjuk cwd
  user; compile dari luar project gagal dengan implicit declaration
  (`semUnsetLocal` dsb) karena menemukan header setengah jalan.
  Root project kini di-resolve dari lokasi binary (`/proc/self/exe`
  → `<root>/bin/`) dengan sanity check `include/rupa.h`; `RUPA_LIB`
  tetap bisa menimpa. Diverifikasi compile+jalankan dari
  `~/rupa/experiments` dan `/tmp`.
- **Empty-body loop peeling** — observasi `rupa profile`: `for i <
  100000 {}` menghabiskan ~97% waktu di stage execute padahal body
  kosong (blok body hanya jump step). Loop for/rev dengan body BLOCK
  kosong, kondisi ident-kiri, bound literal, dan variable belum
  ter-bind kini di-peel saat rewrite: nol iterasi di-emit, cukup
  final store (for: variable = bound) + unbind implisit — IR turun
  dari ~700rb instruksi per loop ke 2–3. Loop lain (body berisi
  statement, while, bound non-literal, variable eksplisit, ident-
  kanan, ber-init) jalan jalur normal. File `tests/codegen/
  loop_empty_peel.rp` ditambahkan (codegen 8→9).
- **Bug revive binding** (ditemukan test codegen baru): tulis ke
  binding yang sudah di-unbind loop (tombstone `dead`) tidak
  menghidupkannya di jalur canon mesin IR — `for i<5 {}; i=0;
  print(i)` mencetak `undefined`, harusnya `0`. `semSetCanon`,
  `semSetConst`, dan `semSetConstCanon` kini sejajar `semSet`:
  menulis menghidupkan ulang binding dead.
- **Test kategori baru `codegen`** — `rupa test codegen`: 7 file di
  `tests/codegen/*.rp` di-compile via backend C lalu output binary
  dibandingkan dengan eksekusi interpreter (harus identik). Cover:
  loop binding implisit/eksplisit, rev loop, loop 50rb iterasi,
  IR_MARK_PUB (pub/private), statement satu baris `;`/`,`, kontrol &
  data (if/while/array/object/call). Runner `src/prompt/test_codegen.c`
  + helper `runnerCaptureStdoutTo` (runner.c).
- **Import member private = ImportError** — `import x from ./mod` saat
  `x` non-pub (mode pub) kini ditolak saat import dengan
  `ImportError: 'x' is private and cannot be imported from './mod'`;
  sebelumnya ter-bind null lalu gagal `TypeError` saat dipanggil.
- Header internal `src/prompt/profile.h` + unit baru `profile_stats.c`,
  `profile_clean.c`, `profile_optimizer.c` — inti profileRun tetap di
  runner.c.
- Jalur mati di `run()` (blok interpreter lama setelah return backend
  IR) dihapus.

### Changed

- **Modularisasi file membengkak** — semua file >1000 line dipecah
  menjadi unit kecil (rules.md: modular, jangan membengkak). Contract
  lintas-unit di header internal; deklarasi publik tetap di header lama
  (lib/prompt/prompt.h, lib/modules/rupa_modules.h, module.h):
  - `src/prompt/test.c` (1157) → `test_internal.h` + 7 unit:
    `test_common.c` (printSource/lexParse), `test_syntax.c` (test+testAst),
    `test_ir.c` (testIR+testIRExec), `test_exec.c`, `test_repl.c`,
    `test_fmt.c`, `test_dispatch.c` (dispatcher + tabel kategori).
  - `src/stdlib/memory.c` (1040) → 4 unit: `memory_new.c` (new/Contract),
    `memory_index.c` (indexing VALUE_PTR), `memory_member.c` (member
    access struct + string slot), `memory_builtin.c` (del/dupl/compare
    + memoryInit).
  - `src/compiler/ir/execute.c` (1039) → `execute_internal.h` (struct
    IRMachine) + 4 unit: `execute_machine.c` (register/get/set/binary),
    `execute_call.c` (execCall + lookup), `execute_function.c`
    (execFunction + handler instruksi), `execute.c` (entry modul).
  - `src/compiler/interpreter/modules/loader.c` (1004) → 4 unit:
    `loader_state.c` (state global, circular guard, module cache),
    `loader_path.c` (resolusi path import), `loader_export.c`
    (export entries), `loader.c` (entry); contract di module.h.
  - (Sesi sebelumnya) `src/compiler/ir/rewrite.c` (1624) → 7 unit;
    `src/formatter/format_normalize.c` (1214) → 3 unit.
  - Fungsi static lintas-unit menjadi non-static dengan prefix
    sesuai domain (`testLexParse`, `modJoinPath`, `modFileExists`,
    dll.) untuk menghindari bentrok dengan helper static TU lain.
- Selesai item: tidak ada lagi file .c/.h di src/ dan lib/ yang
  melebihi 1000 line (terbesar: src/prompt/go.c 833).

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
- **Opcode disassembler (`rupa ir`)** — `src/compiler/ir/debug.c` ditulis
  ulang sebagai disassembler lengkap per opcode untuk tooling:
  - pc 4-digit kontinu antar fungsi; signature `; ---- function f(params)
    -> rettype`; header block `name:  ; N insns`.
  - SEMUA operand tercetak: binary/unary, member/index get-set, call
    (`; argc=N`), jump/branch (`-> block`), alloc (count/elemsz/zeroed),
    realloc, free, strslot get/set, cast. Konstanta string ter-escape
    (`\n`, `\"`, byte non-printable); `IR_VALUE_FUNCTION` tampil `fn:name`;
    tipe array rekursif `T[]`.
  - `IR_CHECK` varian return-type menampilkan `; fn=X node#N` — kontrak
    `function 'f' declared to return ...` kini terlihat di listing.
  - `IR_STRSLOT_GET/SET` kini ikut tercetak (dulu hilang); prefix dobel
    pada ret/free/check diperbaiki; opcode enum yang belum didisasm
    jatuh ke marker `<op ?>` (tidak pernah dicetak diam).
  - Header modul: jumlah function/block/instruction + histogram opcode
    (`; opcodes: add x3 ...`).
- **Build library `librupa.a` + `librupa.so`** — rbot diperbarui
  (v0.1.2, di repo tools/rbot) mendukung produksi library dari object
  build yang sama (`output.libraryName` / `libraryShared` / `libDir`,
  `-fPIC` otomatis saat shared aktif, `exclude` untuk melepas `main.c`
  dari pengemasan tanpa memengaruhi binary). Buildfile rupa kini
  memproduksi `lib/librupa.a` (182 object, tanpa `main.o`) dan
  `lib/librupa.so` (mengekspor API: `debugIRModule`, `semCreateEnv`,
  `stdlibInit`, ...) — `main.c` tetap ikut binary `bin/rupa`.

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
