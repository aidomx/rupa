# Project Structure

Dokumen ini menjelaskan struktur project Rupa dan tanggung jawab setiap bagian utamanya.

Tujuannya bukan menjelaskan detail implementasi setiap file, tetapi membantu memahami di mana sebuah perubahan seharusnya dilakukan.

## Gambaran umum

```text
rupa/
├── bin/        # Binary hasil build
├── build/      # Object dan hasil sementara build
├── commands/   # Library command untuk build.sh
├── docs/       # Dokumentasi project dan bahasa
├── include/    # Public headers
├── lib/        # Internal headers dan deklarasi modul
├── shared/     # Source yang digunakan bersama
├── src/        # Implementasi utama
├── stdlib/     # Standard library Rupa
├── tests/      # Test
├── build.sh    # Sistem build utama
├── Makefile    # Jalur build tambahan
└── README.md   # Informasi awal project
```

## `src/`

`src/` berisi implementasi utama Rupa.

```text
src/
├── bootstrap/
├── compiler/
├── editor/
├── interpreter/
├── repl/
├── runtime/
├── state/
├── utils/
└── main.c
```

### `src/compiler/`

Bagian yang mengubah source Rupa menjadi representasi yang dapat diproses sistem.

```text
compiler/
├── lexer/
├── token/
└── parser/
```

Alur utamanya:

```text
source
  ↓
lexer
  ↓
token
  ↓
parser
  ↓
AST
```

#### `compiler/lexer/`

Bertanggung jawab membaca karakter dan mengenali bagian dasar source.

Di dalamnya terdapat bagian seperti:

```text
lexer/
├── lexeme/      # Pembacaan lexeme
├── processor/   # Pemrosesan construct
├── lexer.c
├── factory.c
├── operations.c
└── support.c
```

Perubahan pada cara karakter, identifier, literal, operator, delimiter, atau keyword dikenali biasanya dimulai dari sini.

#### `compiler/token/`

Bertanggung jawab terhadap representasi token dan operasi yang berkaitan dengannya.

```text
token/
├── check.c
├── error.c
├── lookup.c
├── posix.c
├── save.c
├── symbol.c
└── type.c
```

Gunakan bagian ini ketika perubahan berkaitan dengan jenis, penyimpanan, pencarian, atau validasi token.

#### `compiler/parser/`

Bertanggung jawab mengubah token menjadi struktur AST.

```text
parser/
├── ast/
├── grammar/
└── node/
```

- `grammar/` berisi implementasi aturan construct.
- `ast/` berisi proses pembentukan dan pembacaan struktur AST.
- `node/` berisi pembuatan dan pengelolaan node.

Jika syntax sudah dikenali lexer tetapi belum memiliki struktur program yang benar, masalahnya biasanya berada di area parser.

## `src/runtime/`

`runtime/` menangani sistem yang mendukung eksekusi dan lifecycle program.

```text
runtime/
├── context/
├── gc/
├── input/
├── io/
├── keyword/
└── validation/
```

### `runtime/gc/`

Berisi implementasi Garbage Collector Rupa.

GC digunakan untuk mengelola alokasi yang didaftarkan ke registry memori sehingga dapat dibersihkan secara terpusat.

Perubahan pada lifecycle memori, registry pointer, atau API seperti `gcmall()`, `gcrealloc()`, dan `gcclean()` berada di area ini.

### `runtime/context/`

Menyimpan dan mengelola context yang digunakan selama pemrosesan input atau program.

### `runtime/input/`

Menangani lifecycle input, termasuk pembuatan, validasi, dan pembersihannya.

### `runtime/io/`

Menangani pembacaan input dari sumber seperti file.

### `runtime/validation/`

Berisi validasi yang berkaitan dengan bentuk dan kelengkapan syntax program.

## `src/repl/`

REPL adalah antarmuka interaktif Rupa.

Bagian ini menangani siklus:

```text
input
  ↓
process
  ↓
result
  ↓
input berikutnya
```

Perubahan khusus terhadap perilaku interactive shell sebaiknya berada di sini, bukan dicampurkan ke compiler jika tidak diperlukan oleh source file biasa.

## `src/editor/`

Berisi sistem editor terminal yang digunakan oleh lingkungan interaktif Rupa.

```text
editor/
├── core/
├── display/
└── operations/
```

- `core/` menangani state dasar editor.
- `display/` menangani tampilan terminal.
- `operations/` menangani operasi seperti history, reset, dan indent.

## `src/interpreter/`

Berisi bagian yang berkaitan dengan hasil interpretasi, debugging, dan error.

```text
interpreter/
├── debug/
└── error/
```

## `src/state/`

Menyimpan state global atau state sistem yang digunakan oleh bagian lain.

## `src/utils/`

Berisi utilitas umum yang tidak secara khusus menjadi tanggung jawab compiler, runtime, atau editor.

Contohnya dapat berupa operasi string, number, atom, dan identifier.

Jangan menjadikan `utils/` sebagai tempat untuk kode yang belum diketahui harus diletakkan di mana. Sebuah utilitas tetap harus memiliki tanggung jawab yang cukup umum.

## `lib/`

`lib/` berisi header internal dan deklarasi yang digunakan antar implementasi project.

Struktur `lib/` umumnya mengikuti pembagian modul di `src/`.

Contoh hubungan:

```text
lib/compiler/...   → deklarasi internal compiler
src/compiler/...   → implementasi compiler
```

Header internal tidak harus menjadi bagian dari API publik.

## `include/`

`include/` berisi header yang menjadi bagian dari permukaan API project.

Gunakan direktori ini untuk deklarasi yang memang perlu diakses sebagai interface publik, bukan sekadar untuk semua file header.

## `shared/`

Berisi source yang digunakan bersama oleh beberapa bagian sistem.

Kode di sini sebaiknya benar-benar bersifat shared. Jika kode hanya digunakan oleh satu modul, lebih baik tetap berada dekat dengan modul tersebut.

## `stdlib/`

Berisi standard library Rupa.

Ini adalah tempat untuk kemampuan yang menjadi bagian dari library bahasa, bukan implementasi internal compiler.

## `tests/`

Berisi test untuk perilaku Rupa.

Struktur test dipisahkan berdasarkan area yang diuji. Contoh syntax program berada di:

```text
tests/syntax/
```

Test harus menggambarkan perilaku yang ingin didukung bahasa.

Untuk perubahan syntax, test biasanya lebih penting daripada sekadar memastikan compiler masih berhasil dibangun.

## `docs/`

Dokumentasi dibagi berdasarkan tujuan.

```text
docs/
├── syntax/
├── grammar/
├── structure.md
├── instruction.md
└── ...
```

### `docs/syntax/`

Dokumentasi dari sudut pandang pengguna bahasa.

Fokusnya:

1. Apa yang bisa ditulis?
2. Kapan digunakan?
3. Apa hasilnya?

Gunakan dokumentasi ini sebagai acuan saat memahami atau menulis syntax Rupa.

### `docs/grammar/`

Dokumentasi dari sudut pandang desain bahasa dan sistem.

Fokusnya dapat mencakup:

- struktur grammar
- aturan construct
- relasi antar syntax
- parser
- AST
- alasan desain

### `docs/instruction.md`

Panduan untuk membangun, menjalankan, melakukan test, memahami kontribusi, dan sistem memori project.

### `docs/structure.md`

Dokumen yang sedang dibaca. Gunakan sebagai peta awal untuk memahami lokasi dan tanggung jawab modul.

## `commands/`

Berisi script atau library command yang digunakan oleh `build.sh`.

Bagian ini memungkinkan sistem build dibagi menjadi beberapa tanggung jawab tanpa menumpuk seluruh logika ke dalam satu script.

## `build.sh`

Jalur utama untuk membangun dan menjalankan workflow development Rupa.

Contoh:

```bash
DEV_MODE=1 ./build.sh debug
DEV_MODE=1 ./build.sh test
```

Gunakan:

```bash
DEV_MODE=1 ./build.sh --help
```

untuk melihat command yang tersedia.

## `build/`

Berisi hasil sementara proses build seperti object file.

Direktori ini merupakan output build dan bukan tempat untuk implementasi source utama.

## `bin/`

Berisi executable hasil build, termasuk binary Rupa.

## `Makefile`

Menyediakan jalur build tambahan.

Keberadaannya tidak mengubah pembagian source project; ia hanya menyediakan cara lain untuk menjalankan proses build.

## Cara menentukan lokasi perubahan

Gunakan pertanyaan berikut:

| Perubahan | Lokasi awal |
|---|---|
| Karakter atau lexeme baru | `src/compiler/lexer/` |
| Jenis atau operasi token | `src/compiler/token/` |
| Grammar construct | `src/compiler/parser/grammar/` |
| Bentuk AST | `src/compiler/parser/ast/` atau `node/` |
| Context runtime | `src/runtime/context/` |
| Memory dan GC | `src/runtime/gc/` |
| REPL | `src/repl/` |
| Editor terminal | `src/editor/` |
| Error/debug interpreter | `src/interpreter/` |
| Utilitas umum | `src/utils/` |
| API/header publik | `include/` |
| Header internal | `lib/` |
| Standard library | `stdlib/` |
| Test | `tests/` |
| Dokumentasi syntax | `docs/syntax/` |
| Dokumentasi grammar | `docs/grammar/` |

Jika sebuah perubahan tampak harus menyentuh terlalu banyak bagian, periksa kembali alurnya terlebih dahulu. Bisa jadi perubahan tersebut memang lintas sistem, tetapi bisa juga tanggung jawabnya belum dipisahkan dengan jelas.

## Prinsip membaca project

Jangan membaca seluruh source sekaligus.

Mulai dari:

```text
masalah
  ↓
fitur atau perilaku terkait
  ↓
dokumentasi
  ↓
entry point modul
  ↓
aliran data
  ↓
implementasi terkait
```

Contoh untuk syntax baru:

```text
docs/syntax/
  ↓
docs/grammar/
  ↓
tests/
  ↓
lexer
  ↓
token
  ↓
parser
  ↓
AST
```

Dengan cara ini, struktur project digunakan sebagai peta untuk mengikuti alur sistem, bukan sekadar daftar direktori.
