# 📖 Instruction

## 📦 Dependencies

Sebelum menggunakan **Rupa**, pastikan beberapa paket berikut sudah terpasang di sistem Anda:

- **GCC** → compiler utama
- **Make** → sistem build otomatis
- **Bear** → menghasilkan `compile_commands.json` (untuk tooling seperti clangd)
- **Clangd** (>= 21) → language server untuk fitur editor (intellisense, jump-to-definition, dsb.)

---

## ⚙️ Instalasi

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install gcc make bear clangd-21
```

### Arch Linux

```bash
sudo pacman -S gcc make bear clang
```

### macOS (via Homebrew)

```bash
brew install gcc make bear llvm
```

> **Catatan:** Di macOS, clangd biasanya tersedia setelah instalasi `llvm`. Pastikan `clangd` ada di PATH Anda:
>
> ```bash
> which clangd
> ```

---

## 🚀 Penggunaan

Saat pertama kali membuka project Rupa di editor (misalnya Neovim, VSCode, atau Helix) mungkin akan muncul error terkait header.
Untuk memperbaikinya, jalankan:

```bash
bear -- make
```

Perintah ini menghasilkan file `compile_commands.json` yang memberi tahu clangd bagaimana proyek di-compile, termasuk semua path `include`. Setelah itu, error linting biasanya hilang.

---

## 📂 Struktur Proyek

Berikut gambaran struktur utama proyek **Rupa**:

```
.
├── src/                     # Implementasi utama dalam C
│   ├── ast/                 # Parser dan representasi AST
│   ├── config/              # Konfigurasi internal
│   ├── garbage/             # Garbage collector & manajemen memori
│   ├── io/                  # Input/Output (file, stream, dsb.)
│   ├── repl/                # Implementasi REPL (Read-Eval-Print Loop)
│   ├── tokenize/            # Tokenizer / lexer
│   ├── utils/               # Utilitas umum
│   └── main.c               # Entry point utama
│
└── include/                 # Header files
    └── rupa/                # Header untuk seluruh modul Rupa
        ├── ast/             # Definisi AST
        ├── core/            # Definisi inti (enums, limits, types)
        ├── garbage/         # Header garbage collector
        ├── io/              # Header I/O
        ├── repl/            # Header REPL
        ├── structures/      # Definisi struktur data
        ├── tokenize/        # Header tokenizer
        ├── utils/           # Header utilitas umum
        └── package.h        # Paket utama (pusat semua include)
```

### 🗂️ Catatan Struktur

- Semua header **wajib** berada di `include/rupa/`.
- Setiap modul memiliki **`package.h`** sendiri untuk modularisasi.
- File `rupa/package.h` menjadi pusat agregasi semua paket.
- Direktori `src/` berisi implementasi C sesuai dengan header di `include/rupa/`.

Dengan struktur ini, proyek lebih mudah dipelihara dan konsisten baik untuk REPL maupun File Reader.

---

## 🤝 Kontribusi

Sebelum berkontribusi, ada beberapa hal penting yang perlu diperhatikan:

1. **Satu sumber paket utama**
   Semua paket terpusat pada `rupa/package.h`.

2. **Struktur modular dengan `*/package.h`**
   Jika ingin membuat paket baru, letakkan di `include/rupa/*` dan selalu gunakan `package.h` di dalam folder tersebut.
   Pendekatan ini menjaga konsistensi sekaligus memudahkan modularisasi.

3. **Satu codebase, dua mode**
   Rupa menggunakan satu basis kode yang sama untuk **REPL mode** dan **File Reader mode**, sehingga perubahan tetap sinkron.

4. **Dual mode (High-level & Low-level)**
   Rupa didesain untuk bisa berjalan sebagai bahasa tingkat tinggi sekaligus tingkat rendah, sehingga fleksibel untuk berbagai kebutuhan.

---

## ♻️ Garbage Collector

Rupa telah dilengkapi **garbage collector (GC) internal** untuk mengelola memori.

### Prinsip Dasar

- Semua alokasi memori yang berhubungan dengan **AST, token, atau runtime object** wajib melalui API GC.
- Pengembang **tidak perlu** memanggil `free()` secara manual.
- GC akan menangani pembersihan memori saat objek tidak lagi direferensikan.

### Struktur Direktori

- `src/garbage/` → implementasi GC.
- `include/rupa/garbage/` → header dan API publik.

### Aturan Kontribusi

- **Jangan gunakan `malloc` atau `free` langsung.** Selalu gunakan wrapper yang disediakan GC.
- Jika membuat modul baru, pastikan integrasi memori mengikuti API pada `include/rupa/garbage/`.

### Catatan

- GC saat ini masih dalam tahap pengembangan aktif, optimisasi performa mungkin akan dilakukan.
- Dokumentasi tambahan mengenai fungsi-fungsi GC ada di dalam header terkait (`include/rupa/garbage/package.h`).

✨ Dengan struktur ini, kontribusi Anda akan lebih mudah diintegrasikan ke dalam project Rupa.
