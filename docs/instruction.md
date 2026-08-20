# Development and Contribution Guide

Dokumen ini menjelaskan cara membangun, menjalankan, memahami struktur, dan berkontribusi pada Rupa.

## 1. Build cepat

Dari root repository:

```bash
DEV_MODE=1 ./build.sh debug
```

Binary hasil build berada di:

```text
bin/rupa
```

Jalankan REPL dengan:

```bash
./bin/rupa
```

Mode release, test, atau opsi lain mengikuti command yang tersedia di build script saat ini. Untuk perubahan pada source, `build.sh` adalah jalur build utama karena menangani konfigurasi development dan tooling build proyek. `Makefile` tetap tersedia sebagai build path sederhana.

## 2. Tool yang diperlukan

### Wajib untuk build

- compiler C yang kompatibel dengan proyek, umumnya `gcc`
- shell Bash
- utilitas standar POSIX/Linux seperti `find`

### Disarankan untuk development

- `ccache` untuk mempercepat rebuild
- `bear` atau `intercept-build` untuk menghasilkan `compile_commands.json`
- `clangd` untuk tooling editor
- debugger seperti `gdb` bila diperlukan

Contoh Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential ccache bear clangd
```

Contoh Termux disesuaikan dengan paket yang tersedia di repository Termux.

## 3. Compile commands dan editor

Rupa dapat menggunakan `compile_commands.json` untuk membantu editor dan `clangd` memahami include path serta command compilation.

File tersebut dapat dihasilkan melalui tooling yang didukung `build.sh`, misalnya `bear` atau `intercept-build`. Jangan menganggap versi tooling tertentu sebagai bagian dari grammar atau runtime Rupa; tooling editor adalah pendukung development.

## 4. Struktur proyek saat ini

```text
.
├── bin/                    # Binary hasil build
├── build/                  # Object/build artifacts
├── commands/               # Shell command dan build helpers
│   └── lib/                # Library untuk build scripts
├── docs/                   # Dokumentasi proyek
├── include/                # Header agregat/proyek
├── lib/                    # Header/API internal per modul
├── shared/                 # Shared resources
├── src/                    # Implementasi C
├── stdlib/                 # Standard library Rupa
├── build.sh                # Jalur build utama
├── Makefile                # Jalur build sederhana
└── compile_commands.json   # Database compile untuk tooling, bila tersedia
```

Bagian utama `src/` saat ini:

```text
src/
├── bootstrap/              # Bootstrap/loader
├── compiler/
│   ├── lexer/              # Lexeme, lexer, processor
│   ├── parser/             # Parser dan AST
│   └── token/              # Token type, lookup, storage, error
├── editor/                 # Editor/REPL input handling
├── interpreter/            # Debug/error interpreter
├── prompt/                 # Prompt REPL
├── repl/                   # REPL utama
├── runtime/                # Runtime state, input, validation, GC, keyword
├── state/                  # State program/REPL
├── support/                # Support implementation
└── utils/                  # Utility umum
```

Header internal mengikuti modul di `lib/`. Jangan membuat struktur baru hanya demi mengikuti pola umum proyek lain; ikuti struktur yang sudah digunakan modul terkait kecuali perubahan struktur memang diperlukan.

## 5. Cara memahami alur compiler

Untuk pekerjaan lexer/compiler, urutan umum yang perlu diperhatikan adalah:

```text
Input / REPL state
      ↓
Lexeme scanning
      ↓
Lexer
      ↓
Processor
  ├── keyword handling
  └── construct handling
      ↓
Token stream
      ↓
Parser / AST
```

Tahap lexer saat ini bersifat context-aware. Sebuah karakter seperti `:` tidak selalu memiliki satu arti. Maknanya bergantung pada construct dan context yang sedang aktif.

Contoh:

```rupa
x: number = 1          // type annotation
name: "rupa"           // object property
if x > 0: print(x)     // body satu statement
```

Karena itu, jangan memperbaiki kasus `:` dengan hanya menambah pengecualian berdasarkan nama identifier. Perbaikan harus mengikuti context construct.

## 6. Acuan syntax

Sebelum mengubah:

- keyword
- construct
- token
- delimiter
- operator
- processor
- lexer
- parser
- AST

baca terlebih dahulu [syntax.md](syntax.md).

Contoh bentuk yang saat ini menjadi acuan:

```rupa
if x > 0: print(true)

if x > 0 {
    print(true)
}

People {
    name: string
    age: number
}

people: People = {
    name: "rupa",
    age: 20
}

add(x: number) {
    return x
}
```

`:` dan `{}` memiliki peran berbeda:

- `:` setelah condition/construct control: menunggu satu statement body.
- `{ ... }`: membuka block dengan banyak statement.
- `:` pada declaration: type annotation.
- `:` pada object literal: pemisah property dan value.

Detail lengkap tetap berada di `syntax.md`.

## 7. Aturan kontribusi

### Jangan mengubah lebih dari masalah yang sedang dikerjakan

Jika bug berada pada processor, jangan sekaligus merombak parser, AST, runtime, dan struktur direktori tanpa alasan yang jelas.

### Pertahankan ownership memory

Rupa memiliki runtime GC/internal allocation API. Jangan mencampur allocator biasa dan allocator yang terdaftar pada GC tanpa memahami ownership pointer.

Khususnya, jika pointer dikelola oleh allocator GC, perubahan ukuran buffer harus menggunakan API yang menjaga metadata GC, bukan `realloc()` biasa yang dapat meninggalkan pointer lama di registry.

### Processor boleh dipecah

Processor tidak harus menjadi satu file besar. Pecah berdasarkan tanggung jawab ketika itu membuat context dan komunikasi antar handler lebih jelas. Namun pemecahan file bukan tujuan; state dan alur harus tetap mudah diikuti.

### Keyword dan construct tetap satu pipeline

Keyword dan construct dapat ditangani oleh handler berbeda, tetapi jangan membuat dua lexer yang tidak saling mengetahui state. Context harus dapat diteruskan ketika keyword menghasilkan construct atau construct kembali bertemu keyword.

### Jangan jadikan kasus test sebagai grammar resmi tanpa keputusan

Contoh syntax yang kebetulan lolos lexer belum tentu merupakan syntax Rupa. Tambahkan atau pertahankan grammar berdasarkan `docs/syntax.md` dan keputusan desain, bukan karena sebuah bentuk mirip bahasa lain berhasil diproses.

## 8. Cara menguji perubahan

Gunakan test kecil terlebih dahulu. Untuk perubahan processor `:` misalnya:

```rupa
if x > 0:
print(true)

add(x: number) {
    return x
}
```

Lalu test nested:

```rupa
if x > 0 {
    if y > 1:
    print(y)

    add(x: number) {
        return x
    }
}
```

Untuk struct/object:

```rupa
People {
    name: string
    age: number
}

people: People = {
    name: "rupa",
    age: 20
}
```

Periksa bukan hanya apakah binary tidak crash, tetapi juga apakah token/state bertambah sesuai input dan context sebelumnya tidak bocor ke construct berikutnya.

## 9. Sebelum mengirim perubahan

Minimal lakukan:

```bash
DEV_MODE=1 ./build.sh debug
./bin/rupa
```

Kemudian:

1. Pastikan build sukses.
2. Pastikan perubahan yang dimaksud benar-benar bekerja.
3. Uji input sebelum dan sesudah construct yang diubah.
4. Uji nested context bila fitur mendukung nesting.
5. Pastikan `.exit` tidak memunculkan error memory.
6. Perbarui `docs/syntax.md` jika grammar resmi berubah.
7. Perbarui dokumentasi lain jika struktur atau workflow berubah.

## 10. Titik awal berdasarkan jenis kontribusi

| Pekerjaan | Mulai dari |
|---|---|
| Syntax baru | `docs/syntax.md`, lalu lexer/processor |
| Keyword baru | `src/compiler/lexer/processor/` dan runtime keyword terkait |
| Construct baru | `src/compiler/lexer/processor/` |
| Token baru | `src/compiler/token/` |
| Parser/AST | `src/compiler/parser/` |
| REPL/editor | `src/editor/`, `src/repl/`, `src/prompt/` |
| Runtime/input | `src/runtime/` |
| Build | `build.sh`, `commands/`, `commands/lib/` |
| Dokumentasi | `docs/` |

Jika ragu, mulai dari `docs/syntax.md` untuk perilaku bahasa dan `instruction.md` ini untuk lokasi implementasi.
