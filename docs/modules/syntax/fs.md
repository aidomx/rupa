# Fs

Modul `fs` menyediakan operasi filesystem: baca/tulis file, baris, path helper, copy/move. Level primitive dibawa dari modul native `fsbase`, level tinggi ditulis murni dengan bahasa Rupa.

```rupa
import fs from rupa
```

## Baca & tulis

```rupa
fs.write("data.txt", "hello\n")
text = fs.read("data.txt")
print(text)
// hello
```

`fs.read()` membaca seluruh isi file sebagai string. `fs.write()` menimpa isi file.

### Append

```rupa
fs.append("log.txt", "baris baru\n")
```

## Baris

```rupa
lines = fs.readLines("data.txt")
print(lines)
// ["hello", "world"]

fs.writeLines("out.txt", ["a", "b"])
// out.txt berisi "a\nb\n"
```

## Path helper

```rupa
print(fs.base("a/b/c.txt"))  // c.txt
print(fs.dir("a/b/c.txt"))   // a/b
print(fs.ext("a/b/c.txt"))   // txt
print(fs.join("a", "b.txt")) // a/b.txt
```

## Copy, move, remove

```rupa
fs.copy("a.txt", "b.txt")    // true jika sukses
fs.move("b.txt", "c.txt")    // copy + remove sumber
fs.remove("c.txt")           // true jika sukses
```

## Info file

```rupa
print(fs.exists("a.txt"))  // true/false
print(fs.size("a.txt"))    // ukuran byte, -1 jika tidak ada
print(fs.isDir("."))       // true jika direktori
```
