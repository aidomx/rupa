# Development and Contribution Guide

Dokumen ini menjelaskan cara membangun, menjalankan, memahami proyek, dan berkontribusi pada Rupa.

## 1. Memulai

Clone repository Rupa dan masuk ke root project. Dari sana, jalur utama untuk development adalah `build.sh`.

Build dalam mode development:

```bash
DEV_MODE=1 ./build.sh debug
```

Binary hasil build berada di:

```text
bin/rupa
```

Jalankan Rupa dengan:

```bash
./bin/rupa
```

Untuk melihat command yang tersedia:

```bash
DEV_MODE=1 ./build.sh --help
```

`Makefile` tetap tersedia sebagai jalur build tambahan, tetapi `build.sh` digunakan sebagai jalur utama development.

## 2. Syarat yang diperlukan

### Wajib

- Compiler C yang kompatibel, umumnya `gcc`
- Bash
- Utilitas dasar sistem Linux/POSIX

### Disarankan untuk development

- `ccache` untuk mempercepat build ulang
- `bear` atau `intercept-build` untuk `compile_commands.json`
- `clangd` untuk tooling editor
- `gdb` untuk debugging

Contoh pada Debian:

```bash
sudo apt update
sudo apt install build-essential ccache bear clangd gdb
```

Sesuaikan nama paket dengan distribusi atau environment yang digunakan.

## 3. Cara build dan test

Build development:

```bash
DEV_MODE=1 ./build.sh debug
```

Jalankan test:

```bash
DEV_MODE=1 ./build.sh test
```

Jika ingin melakukan perubahan pada compiler atau syntax, jalankan test setelah perubahan dibuat.

Untuk perubahan yang lebih besar, gunakan urutan sederhana:

```text
ubah source
   ↓
build
   ↓
test
   ↓
jalankan contoh secara langsung bila diperlukan
```

Jangan hanya menganggap perubahan benar karena project berhasil dikompilasi. Build yang sukses tidak selalu berarti perilaku bahasa sudah benar.

## 4. Memahami struktur project

Struktur direktori dan tanggung jawab setiap bagian project dijelaskan di:

[structure.md](structure.md)

Baca dokumen tersebut sebelum memindahkan file, membuat modul baru, atau mengubah batas tanggung jawab antar bagian sistem.

Secara umum, Rupa terdiri dari beberapa alur utama:

```text
Source / REPL
      ↓
Lexer
      ↓
Processor
      ↓
Token
      ↓
Parser
      ↓
AST
      ↓
Runtime
```

Setiap bagian memiliki tanggung jawab sendiri. Perubahan pada satu tahap tidak selalu harus menyebabkan perubahan pada seluruh tahap.

## 5. Memahami dan menulis syntax Rupa

Dokumentasi syntax ditujukan dari sudut pandang pengguna bahasa.

Mulai dari:

[docs/syntax/](syntax/)

Setiap dokumen syntax berfokus pada tiga hal:

1. Apa yang bisa ditulis?
2. Kapan digunakan?
3. Apa hasilnya?

Contohnya:

```text
docs/syntax/
├── assignment.md
├── function.md
├── if.md
├── loop.md
├── object.md
└── ...
```

Jika ingin memahami bagaimana syntax tersebut dibentuk oleh sistem, bagaimana parser membaca construct, atau bagaimana bentuk AST-nya, lihat:

[docs/grammar/](grammar/)

Pembagiannya adalah:

```text
docs/syntax/
└── Perspektif pengguna bahasa

docs/grammar/
└── Perspektif desain bahasa dan sistem
```

Jangan menggunakan implementasi C sebagai satu-satunya acuan untuk menentukan syntax resmi. Syntax yang didukung dan cara penggunaannya harus memiliki acuan di dokumentasi syntax.

## 6. Sistem memori dan GC

Rupa memiliki sistem pengelolaan memori terpusat melalui `GarbageCollector`.

GC Rupa saat ini bekerja sebagai registry kepemilikan alokasi. Pointer yang dialokasikan atau didaftarkan ke GC disimpan dalam daftar internal sehingga dapat dibersihkan secara terpusat.

Alur sederhananya:

```text
gcinit()
   ↓
alokasi melalui API GC
   ↓
pointer didaftarkan ke GC
   ↓
runtime menggunakan pointer
   ↓
gcclean()
   ↓
seluruh pointer terdaftar dibersihkan
```

API utama:

```c
gcinit(capacity);              // Membuat konteks GC

gcmall(size);                  // malloc + register ke GC
gccalloc(count, size);         // calloc-like + register
gcrealloc(ptr, size);          // realloc sambil memperbarui registry
gcresize(ptr, old, new);       // resize + zero-initialize bagian baru

gcstrdup(str);                 // duplicate string yang dikelola GC
gcstrndup(str, n);             // duplicate string dengan panjang tertentu
gcarray(count, size);          // alokasi array

gcreg(ptr);                    // register pointer
gcremove(ptr);                 // hapus dari registry tanpa free
gcfree(ptr);                   // free pointer dan hapus dari registry
gcfind(ptr);                   // mencari pointer di registry

gcclean();                     // membersihkan seluruh pointer terdaftar
```

### Ownership

Aturan pentingnya adalah memahami siapa yang memiliki pointer.

Jika memori dibuat dengan:

```c
void *data = gcmall(size);
```

pointer tersebut telah terdaftar pada GC. Jangan memperlakukannya sebagai alokasi biasa tanpa memperhatikan registry.

Khusus saat ukuran pointer berubah, gunakan:

```c
data = gcrealloc(data, new_size);
```

bukan langsung:

```c
data = realloc(data, new_size);
```

Alasannya, `realloc()` dapat menghasilkan alamat baru. Jika registry GC masih menyimpan alamat lama, registry dan pointer aktual akan tidak sinkron.

`gcrealloc()` memperbarui pointer di registry ketika alamat berubah.

### Pembersihan manual

Jika sebuah alokasi ingin dibersihkan sebelum seluruh runtime selesai:

```c
gcfree(data);
```

Jika pointer tidak lagi ingin dikelola GC tetapi tidak ingin langsung dibebaskan:

```c
gcremove(data);
```

Setelah pointer dihapus dari registry, ownership-nya harus ditangani oleh bagian lain. Jangan membiarkan pointer kehilangan pemilik yang jelas.

### Prinsip kontribusi untuk memori

Sebelum menambahkan `malloc()`, `calloc()`, `realloc()`, atau `free()`, tanyakan:

- Siapa pemilik memori ini?
- Berapa lama memori harus hidup?
- Apakah memori ini harus terdaftar pada GC?
- Siapa yang membebaskannya?
- Apakah pointer dapat berubah alamat?

Jangan mencampur alokasi biasa dan alokasi yang dikelola GC tanpa memahami ownership-nya.

## 7. Cara berkontribusi

Sebelum mengubah kode, tentukan terlebih dahulu masalah dan tahap sistem yang benar-benar terkait.

Contoh:

```text
Syntax
  ↓
docs/syntax/
  ↓
Lexer / Processor
  ↓
Token
  ↓
Parser / AST
  ↓
Runtime
```

Tidak semua perubahan harus melewati seluruh jalur tersebut.

### Perubahan syntax

Jika ingin menambah atau mengubah syntax:

1. Baca `docs/syntax/*.md`.
2. Tentukan perilaku yang ingin didukung pengguna.
3. Periksa `docs/grammar/*.md` untuk dampaknya terhadap grammar dan sistem.
4. Ubah implementasi yang memang diperlukan.
5. Tambahkan atau perbarui test.
6. Jalankan build dan test.
7. Perbarui dokumentasi jika syntax resmi berubah.

### Perubahan internal

Untuk perubahan lexer, parser, AST, runtime, atau modul internal:

1. Pahami alur data dan context yang sudah ada.
2. Temukan titik ownership dan tanggung jawab modul.
3. Ubah bagian yang relevan terlebih dahulu.
4. Hindari merombak bagian lain tanpa kebutuhan yang jelas.
5. Build dan test setelah perubahan.

Struktur project yang ada lebih penting daripada memaksakan pola umum dari project lain. Perubahan struktur harus dilakukan karena memang menyelesaikan masalah arsitektur, bukan hanya karena terlihat lebih familiar.

## 8. Sebelum mengirim perubahan

Minimal lakukan:

```bash
DEV_MODE=1 ./build.sh debug
DEV_MODE=1 ./build.sh test
```

Kemudian pastikan:

1. Build berhasil.
2. Test terkait berhasil.
3. Perubahan menghasilkan perilaku yang memang diinginkan.
4. Context tidak bocor ke input atau construct berikutnya.
5. Ownership memori tetap jelas.
6. Dokumentasi diperbarui jika perilaku resmi berubah.
7. Perubahan tidak mencampurkan masalah yang tidak berkaitan.

## 9. Titik awal

| Jenis pekerjaan | Mulai dari |
|---|---|
| Memahami project | `docs/structure.md` |
| Memahami syntax | `docs/syntax/*.md` |
| Memahami grammar dan sistem | `docs/grammar/*.md` |
| Mengubah lexer | `src/compiler/lexer/` |
| Mengubah parser atau AST | `src/compiler/parser/` |
| Mengubah token | `src/compiler/token/` |
| Mengubah runtime | `src/runtime/` |
| Mengubah GC | `src/runtime/gc/` dan `lib/runtime/gc/` |
| Mengubah build | `build.sh` dan `commands/` |
| Menambah atau memperbarui test | `tests/` |

Jika tidak yakin harus mulai dari mana, mulai dari dokumentasi terlebih dahulu. Pahami apa yang seharusnya dilakukan sistem sebelum mengubah bagaimana sistem melakukannya.
