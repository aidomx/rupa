# Regex Module

Module `regex` menyediakan fungsi regular expression menggunakan POSIX regex.

```rupa
import regex from rupa
```

## Fungsi

### regex.match(pattern, str)
Mengembalikan `true` jika string cocok dengan pattern.

```rupa
print(regex.match("[0-9]+", "hello123"))  // true
print(regex.match("[0-9]+", "hello"))     // false
```

### regex.find(pattern, str)
Mengembalikan match pertama, atau `null`.

```rupa
found = regex.find("[0-9]+", "abc123def456")
print(found)  // 123
```

### regex.findAll(pattern, str)
Mengembalikan array semua match.

```rupa
all = regex.findAll("[0-9]+", "abc123def456")
print(all)  // ["123", "456"]
```

### regex.replace(pattern, str, replacement, global?)
Replace match. Parameter `global` (default: `false`) replace semua match.

```rupa
print(regex.replace("[0-9]+", "abc123def456", "NUM"))
// abcNUMdef456

print(regex.replace("[0-9]+", "abc123def456", "NUM", true))
// abcNUMdefNUM
```

### regex.split(pattern, split)
Split string berdasarkan pattern.

```rupa
parts = regex.split("[,;]+", "one,two;three,four")
print(parts)  // ["one", "two", "three", "four"]
```

| Fungsi | Parameter | Return |
|--------|-----------|--------|
| `match(pattern, str)` | dua string | boolean |
| `find(pattern, str)` | dua string | string/null |
| `findAll(pattern, str)` | dua string | array |
| `replace(pattern, str, repl, global?)` | string, string, string, boolean? | string |
| `split(pattern, str)` | dua string | array |

## Pattern Syntax (POSIX Extended)

| Pattern | Arti |
|---------|------|
| `.` | Karakter apapun |
| `[0-9]` | Digit |
| `[a-z]` | Huruf lowercase |
| `[A-Z]` | Huruf uppercase |
| `[a-zA-Z]` | Huruf |
| `+` | Satu atau lebih |
| `*` | Nol atau lebih |
| `?` | Nol atau satu |
| `{n,m}` | n sampai m kali |
| `^` | Awal string |
| `$` | Akhir string |
| `(group)` | Grup |
| `a\|b` | Alternatif |
