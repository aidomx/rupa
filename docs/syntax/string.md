# String

## Apa yang bisa ditulis?

```rupa
"hello world"
"rupa"
```

String concatenation:

```rupa
"hello" + " " + "world"
```

String dengan variable:

```rupa
name = "rupa"
print(name + " language")
```

## Method string

Method string bawaan dibuat minimal, sejajar dengan array (`push`/`pop`) — hanya operasi yang memang sifatnya bawaan tipe. Transformasi yang lebih kompleks tetap di modul `strings`.

### split

Memecah string berdasarkan separator menjadi array:

```rupa
"a,b,c".split(",")
// ["a", "b", "c"]

"2026-09-13".split("-")
// ["2026", "09", "13"]
```

### indexOf

Posisi pertama substring, atau `-1` jika tidak ditemukan:

```rupa
"hello".indexOf("lo")   // 3
"hello".indexOf("z")    // -1
```

### slice

Potong string `slice(start, end?)` — `end` opsional, index negatif dihitung dari akhir:

```rupa
"hello".slice(1)      // "ello"
"hello".slice(0, -1)  // "hell"
"hello".slice(-1)     // "o"
```

### Method lain

```rupa
"rupa".upper()                    // "RUPA"
"RUPA".lower()                    // "rupa"
"  hi  ".trim()                   // "hi"
"hello".contains("ell")           // true
"hello".startsWith("he")          // true
"hello".endsWith("lo")            // true
"hello".replace("l", "L")         // "heLlo" (satu penggantian pertama)
```

### Indexing

Satu karakter dapat diakses dengan subscript:

```rupa
"abc"[1]
// "b"
```

## Kapan digunakan?

Gunakan string untuk menyimpan dan memanipulasi teks. String dibuat dengan tanda kutip `"`.

## Apahasilnya?

String adalah tipe data teks. Operator `+` digunakan untuk concatenation.

### Contoh execution

```rupa
s = "hello" + " " + "world"
print(s)
```

Output: `hello world`

```rupa
name = "rupa"
print(name + " language")
```

Output: `rupa language`

```rupa
x = 1
print("x =", x)
```

Output: `x = 1`

```rupa
x = 1
y = 2
print("result:", x + y, true)
```

Output: `result: 3 true`
