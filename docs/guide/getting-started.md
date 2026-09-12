# Memulai

Panduan ini menjelaskan cara membangun dan menjalankan Rupa dari source.

## Syarat

### Wajib

- Compiler C yang kompatibel, umumnya `gcc`
- Bash
- Utilitas dasar sistem Linux/POSIX

### Disarankan untuk development

- `ccache` untuk mempercepat build ulang
- `make` sebagai jalur build tambahan
- `gdb` untuk debugging

Contoh pada Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential libssl-dev ccache make gdb
```

## Library yang digunakan

Proses link di `Makefile` menggunakan empat library sistem:

```make
LD_FLAGS = -lm -lpthread -lssl -lcrypto
```

| Library | Fungsi |
|---------|--------|
| `libm` (`-lm`) | Fungsi matematika C (`sqrt`, `pow`, `sin`, `cos`) — dipakai module `math` |
| `libpthread` (`-lpthread`) | Thread POSIX — dipakai module `thread`, async/event loop, dan GC lock |
| `libssl` + `libcrypto` (`-lssl -lcrypto`) | OpenSSL — dipakai module `crypto` (hash, base64) dan `http`/`net` (TLS) |

`libssl-dev` wajib terpasang sebelum build; tanpa itu link akan gagal. Contoh di atas sudah menyertakannya.

Selain library sistem, build juga meng-embed stdlib Rupa ke binary: semua file `stdlib/*.rp` diarsipkan ke `modules/rupa_modules.tar.gz`, dikonversi menjadi object file via `ld -r -b binary`, lalu di-link bersama binary sehingga `import ... from rupa` bekerja tanpa file eksternal.

## Build dari source

Clone repository dan masuk ke root project:

```bash
git clone https://github.com/aidomx/rupa.git
cd rupa
```

Build mode development menggunakan `build.sh`:

```bash
DEV_MODE=1 ./build.sh debug
```

Binary hasil build berada di `bin/rupa`. `Makefile` tetap tersedia sebagai jalur tambahan (`make`), tetapi `build.sh` adalah jalur utama development.

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

```bash
DEV_MODE=1 ./build.sh test
```

Jalankan test tertentu:

```bash
DEV_MODE=1 ./build.sh test --select "3"
```

## Langkah berikutnya

- [Referensi Syntax](/syntax/) — semua syntax yang tersedia
- [Module](/modules/syntax/math) — stdlib bawaan: os, io, json, thread, math, http, dan lainnya
- [Instruction](/instruction) — cara membangun, menjalankan, dan berkontribusi pada proyek
