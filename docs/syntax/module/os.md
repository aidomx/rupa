# Os

Modul `os` menyediakan fungsi untuk berinteraksi dengan sistem operasi.

```rupa
import os from rupa
```

## Apa yang bisa dilakukan?

### Mendapatkan working directory

```rupa
cwd = os.getcwd()
print(cwd)
// /home/user/project
```

### Mengganti working directory

```rupa
os.chdir("/tmp")
print(os.getcwd())
// /tmp
```

### Menjalankan command sistem

```rupa
output = os.exec("ls -la")
print(output)
```

`os.exec()` mengembalikan output dari command sebagai string.

```rupa
// Cek versi node
version = os.exec("node --version")
print(version)

// Cek disk usage
disk = os.exec("df -h /")
print(disk)
```

### Membaca environment variable

```rupa
user = os.getenv("USER")
print(user)
// aidomx

// Jika variable tidak ada, mengembalikan null
home = os.getenv("HOME")
print(home)
// /home/aidomx
```

### Bekerja dengan file dan directory

#### Membuat directory

```rupa
os.mkdir("my-folder")
print(os.path.exists("my-folder"))
// true
```

#### Cek keberadaan file/directory

```rupa
print(os.path.exists("/etc/passwd"))
// true

print(os.path.exists("file-tidak-ada"))
// false
```

#### List isi directory

```rupa
files = os.listdir(".")
print(files)
// [src, bin, lib, ...]
```

#### Rename file

```rupa
os.rename("old-name.txt", "new-name.txt")
```

#### Hapus file

```rupa
os.remove("file-yang-ingin-dihapus.txt")
```

### Informasi sistem

Tanpa parameter, `os.info()` mengembalikan object dengan semua informasi:

```rupa
info = os.info()

print(info.sysname)    // Linux
print(info.machine)    // x86_64
print(info.release)    // 6.1.0-generic
print(info.user)       // aidomx
print(info.hostname)   // mycomputer
```

Dengan parameter, `os.info(key)` mengembalikan string tertentu:

```rupa
print(os.info("machine"))
// x86_64

print(os.info("user"))
// aidomx

print(os.info("sysname"))
// Linux
```

Tersedia keys: `sysname`, `nodename`, `release`, `version`, `machine`, `user`, `hostname`.

### Keluar dari program

```rupa
os.exit(0)
```

## Ringkasan

| Fungsi | Parameter | Return | Deskripsi |
|--------|-----------|--------|-----------|
| `getcwd()` | - | string | Working directory |
| `chdir(path)` | string | bool | Ganti directory |
| `listdir(path)` | string | array | List isi directory |
| `getenv(key)` | string | string/null | Baca env variable |
| `exec(cmd)` | string | string | Jalankan command |
| `mkdir(path)` | string | bool | Buat directory |
| `remove(path)` | string | bool | Hapus file |
| `rename(old, new)` | string×2 | bool | Rename file |
| `info(key?)` | string (opt) | object/string | Info sistem |
| `exit(code)` | int | - | Keluar program |

### os.path

| Fungsi | Parameter | Return | Deskripsi |
|--------|-----------|--------|-----------|
| `exists(path)` | string | bool | Cek keberadaan |

## Contoh

### Backup file

```rupa
import os from rupa

// Cek file ada
if os.path.exists("config.txt"):
    // Rename ke backup
    os.rename("config.txt", "config.txt.bak")
    print("File backed up")
else:
    print("File not found")
```

### Setup project

```rupa
import os from rupa

// Buat struktur folder
os.mkdir("src")
os.mkdir("tests")
os.mkdir("docs")

// Cek hasil
print(os.listdir("."))
// [src, tests, docs, ...]

print("Project structure created!")
```

### Cek environment

```rupa
import os from rupa

print("User:", os.getenv("USER"))
print("Home:", os.getenv("HOME"))
print("Shell:", os.getenv("SHELL"))
print("Platform:", os.info("sysname"))
print("Architecture:", os.info("machine"))
```
