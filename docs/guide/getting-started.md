# Memulai

Panduan ini menjelaskan cara meng-install rbot, membangun, dan menjalankan Rupa dari source.

## Install rbot

Rupa dibangun menggunakan [rbot](https://github.com/aidomx/rbot) — build tool berbasis konfigurasi `Buildfile`. rbot wajib terpasang sebelum build:

```bash
curl -fsSL https://raw.githubusercontent.com/aidomx/rbot/main/install.sh | sh
```

Atau kunjungi reponya untuk build dari source:

```text
https://github.com/aidomx/rbot
```

Verifikasi instalasi:

```bash
rbot --version
```

## Syarat

### Wajib

- rbot (lihat langkah di atas)
- Compiler C yang kompatibel, umumnya `gcc` atau `clang`
- Utilitas dasar sistem Linux/POSIX

### Disarankan untuk development

- `ccache` untuk mempercepat build ulang
- `gdb` untuk debugging

Contoh pada Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential libssl-dev ccache gdb
```

## Library yang digunakan

Proses link yang dijalankan rbot (sesuai `Buildfile`) menggunakan empat library sistem:

| Library | Fungsi |
|---------|--------|
| `libm` (`lm`) | Fungsi matematika C (`sqrt`, `pow`, `sin`, `cos`) — dipakai module `math` |
| `libpthread` (`pthread`) | Thread POSIX — dipakai module `thread`, async/event loop, dan GC lock |
| `libssl` + `libcrypto` (`ssl`, `crypto`) | OpenSSL — dipakai module `crypto` (hash, base64) dan `http`/`net` (TLS) |

`libssl-dev` wajib terpasang sebelum build; tanpa itu link akan gagal. Contoh di atas sudah menyertakannya.

Selain library sistem, build juga meng-embed stdlib Rupa ke binary: seluruh file `stdlib/*.rp` diarsipkan oleh rbot (bagian `embedded` pada `Buildfile`) ke `modules/rupa_modules.tar.gz`, dikonversi menjadi object file, lalu di-link bersama binary sehingga `import ... from rupa` bekerja tanpa file eksternal.

## Build dari source

Clone repository dan masuk ke root project:

```bash
git clone https://github.com/aidomx/rupa.git
cd rupa
```

Build menggunakan rbot. Konfigurasi build dibaca dari `Buildfile` di root project:

```bash
rbot
```

rbot memilih compiler yang tersedia (`gcc`/`clang`), mengompilasi seluruh source di `src/`, menyiapkan `compile_commands.json`, dan menulis binary ke:

```text
bin/rupa
```

Perintah rbot lain yang berguna:

```bash
rbot clean   # bersihkan artefak build (aturan: Buildfile: clean)
rbot init    # buat Buildfile default jika belum ada
rbot help    # bantuan rbot
```

## Program pertama

Buat file `hello.rp`:

```rupa
print("Halo, Rupa!\n")
```

Jalankan:

```bash
./bin/rupa hello.rp
```

Output:

```text
Halo, Rupa!
```

Tanpa argumen file, Rupa masuk ke REPL interaktif:

```bash
./bin/rupa
```

## Formatter

Rupa menyertakan formatter bawaan yang merapikan spasi operator, comment, dan indentasi:

```bash
./bin/rupa fmt hello.rp
```

Format dari stdin (berguna untuk integrasi editor seperti Vim):

```bash
./bin/rupa fmt - < hello.rp
```

Contoh sebelum dan sesudah:

```rupa
x=1
y=primary|fallback|default
z?=true->x
```

Menjadi:

```rupa
x = 1
y = primary | fallback | default
z ?= true -> x
```

## Menjalankan test

Test dijalankan langsung lewat binary rupa. Kategori test: `syntax` (default), `ast`, `ir`, `irexec`, `exec`, `semantics`, `repl`, dan `fmt`.

```bash
./bin/rupa test                  # jalankan tests/syntax/*.rp
./bin/rupa test ast              # jalankan tests/ast/*.rp
./bin/rupa test exec             # jalankan tests/execution/*.rp
./bin/rupa test fmt              # source asli vs hasil formatter
```

Melihat daftar file test:

```bash
./bin/rupa test --list           # semua tests/**/*.rp
./bin/rupa test --list ast       # hanya tests/ast/*.rp
```

Menjalankan sebagian file lewat nomor pada `--list`:

```bash
./bin/rupa test --select 1,3     # filter dari tests/syntax/*.rp
./bin/rupa test --path ast --select 1
```

Flag dan kategori boleh dikombinasikan dalam urutan apa pun. Tersedia juga bentuk pendek:

```bash
./bin/rupa -t                    # sama dengan: rupa test
./bin/rupa -t -p syntax -s 1     # -p = --path, -s = --select
./bin/rupa -tps syntax 1         # gabungan cluster: -t -p -s
./bin/rupa -ts ast 1             # test ast, pilih file 1
./bin/rupa -l                    # sama dengan: rupa test --list
./bin/rupa -lp ast               # -l -p ast: daftar tests/ast/*.rp
```

Untuk test satu file tertentu (tanpa batch):

```bash
./bin/rupa --test tests/syntax/module.rp
./bin/rupa --test-ir tests/syntax/module.rp
```

Jika ingin melakukan perubahan pada compiler atau syntax, jalankan test setelah perubahan dibuat. Build yang sukses tidak selalu berarti perilaku bahasa sudah benar.

## Langkah berikutnya

- [Referensi Syntax](/syntax/) — semua syntax yang tersedia
- [Module](/modules/syntax/math) — stdlib bawaan: os, io, json, thread, math, http, dan lainnya
- [Instruction](/instruction) — cara membangun, menjalankan, dan berkontribusi pada proyek
