# Sys

Modul `sys` menyediakan informasi sistem dan utilitas path. Ditulis murni dengan bahasa Rupa di atas modul native `os`.

```rupa
import sys from rupa
```

## Informasi sistem

### Environment variable

```rupa
home = sys.env("HOME")
print(home)
// /home/user
```

### Working directory & platform

```rupa
print(sys.cwd())        // /home/user/project
print(sys.platform())   // Linux
print(sys.arch())       // aarch64 / x86_64
print(sys.hostname())   // nama host mesin
print(sys.user())       // nama user aktif
```

### Path khusus

```rupa
print(sys.home())       // /home/user
print(sys.tmp())        // /tmp
```

### Cek keberadaan path

```rupa
if sys.exists("./config.rp"):
    print("ada")
```

## Utilitas path

Semua fungsi path bekerja dengan gaya POSIX (`/`).

```rupa
print(sys.sep())                    // /
print(sys.join("a", "b.txt"))       // a/b.txt
print(sys.base("a/b/c.txt"))        // c.txt
print(sys.dir("a/b/c.txt"))         // a/b
print(sys.ext("a/b/c.txt"))         // txt
print(sys.normalize("a/./b/../c"))  // a/c
print(sys.isAbsolute("/tmp"))       // true
```

## Namespace path

Utilitas path juga tersedia sebagai sub-object agar rapi:

```rupa
print(sys.path.join("a", "b"))
```
