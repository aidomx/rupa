# Syntax

Dokumen ini menjelaskan dasar penulisan kode dalam **Rupa Language**.
Fokus saat ini adalah bentuk penulisan, belum mencakup interpreter maupun validasi penuh.

---

## Assignment

```rupa
x[] = null
x = 1
x = x
```

- `x[] = null` → assignment dengan indeks/array kosong.
- `x = 1` → assignment sederhana.
- `x = x` → assignment yang membedakan **Identifier** di sisi kiri dengan **Literal Identifier** di sisi kanan.

### AST Contoh:

```
--- Struktur AST Node ---
Program:
  Assignment:
    Target:
      Subscript:
        Base:
          Identifier: x
          SafetyType: auto
        Index:
          (empty)
    Value:
      Nullable: null
  Assignment:
    Target:
      Identifier: x
      SafetyType: auto
    Value:
      Number: 1
  Assignment:
    Target:
      Identifier: x
      SafetyType: auto
    Value:
      Literal ID: x
--- ENDOF ---
```

---

## Typed Assignment

```rupa
x: number = 1
```

- `: number` → deklarasi tipe variabel.
- Rupa mendukung tipe dasar: `null`, `true`, `false`, `number`, `string`, `float`.

### AST Contoh:

```
--- Struktur AST Node ---
Program:
  Assignment:
    Target:
      Identifier: x
      SafetyType: number
    Value:
      Number: 1
--- ENDOF ---
```

---

## Expression

```rupa
x = (1+2)
x = ((1+2)*3)/((4+5)-6)
x = 5 % 2
```

- Mendukung operasi aritmetika dasar:

  - Penjumlahan (`+`)
  - Pengurangan (`-`)
  - Perkalian (`*`)
  - Pembagian (`/`)
  - Modulus (`%`)
  - Tanda kurung `()` untuk prioritas operasi

### AST Contoh:

```
--- Struktur AST Node ---
Program:
  Assignment:
    Target:
      Identifier: x
      SafetyType: auto
    Value:
      Binary: +
        Left:
          Number: 1
        Right:
          Number: 2
  Assignment:
    Target:
      Identifier: x
      SafetyType: auto
    Value:
      Binary: *
        Left:
          Binary: +
            Left:
              Number: 1
            Right:
              Number: 2
        Right:
          Number: 3
  Assignment:
    Target:
      Identifier: x
      SafetyType: auto
    Value:
      Binary: %
        Left:
          Number: 5
        Right:
          Number: 2
--- ENDOF ---
```

---

## Array Literal

- [x] Array kosong → `[]`
- [x] Array dengan elemen literal → `[1, 2, 3]`
- [x] Array dengan ekspresi → `[1+2, 3*4]`
- [x] Nested array → `[1, [2, 3], 4]`
- [ ] Spread element (`[...arr, 5]`) → _planned_
- [ ] Destructuring assignment (`[a, b] = [1,2]`) → _planned_

---

## Subscript & Nested Access

- [x] Akses elemen tunggal → `x[0]`
- [x] Nested subscript (array dalam array) → `x[0][1]`
- [x] Nested lebih dalam → `x[0][1][2]`
- [x] Subscript dengan ekspresi aritmetika → `x[1+2]`
- [x] Subscript dengan identifier → `x[y]`
- [x] Kombinasi nested & ekspresi → `x[1+2][y]`
- [ ] Optional chaining (`x?[0]`) → _planned_
- [ ] Negative index (`x[-1]`) → _planned_

### Contoh

```rupa
x[0] = 1
x[0][1] = 2
x[0][1][2] = 3
x[1+2][y] = ((1+2)*3)
```

### AST Contoh

```
--- Struktur AST Node ---
Program:
  Assignment:
    Target:
      Subscript:
        Base:
          Identifier: x
          SafetyType: auto
        Index:
          (empty)
    Value:
      Number: 1
  Assignment:
    Target:
      Subscript:
        Base:
          Identifier: x
          SafetyType: auto
        Index:
          Number: 0
    Value:
      Number: 2
  Assignment:
    Target:
      Subscript:
        Base:
          Subscript:
            Base:
              Identifier: x
              SafetyType: auto
            Index:
              Number: 0
        Index:
          Number: 1
    Value:
      Number: 3
  Assignment:
    Target:
      Subscript:
        Base:
          Subscript:
            Base:
              Subscript:
                Base:
                  Identifier: x
                  SafetyType: auto
                Index:
                  Number: 0
            Index:
              Number: 1
        Index:
          Number: 2
    Value:
      Number: 4
  Assignment:
    Target:
      Subscript:
        Base:
          Subscript:
            Base:
              Identifier: x
              SafetyType: auto
            Index:
              Binary: +
                Left:
                  Number: 1
                Right:
                  Number: 2
        Index:
          Literal ID: y
    Value:
      Binary: *
        Left:
          Binary: +
            Left:
              Number: 1
            Right:
              Number: 2
        Right:
          Number: 3
--- ENDOF ---
```

---

## Tipe Data yang Didukung

- `null`
- `true` / `false` (boolean)
- `number` (bilangan bulat)
- `float` (bilangan desimal)
- `string` (teks)

---

# Roadmap Syntax

Fitur berikut **belum sepenuhnya tersedia** tapi sudah dalam tahap desain:

## Array Literal

- [x] Array kosong → `[]`
- [x] Array dengan elemen literal → `[1, 2, 3]`
- [x] Array dengan ekspresi → `[1+2, 3*4]`
- [x] Nested array → `[1, [2, 3], 4]`
- [ ] Spread element (`[...arr, 5]`) → _planned_
- [ ] Destructuring assignment (`[a, b] = [1,2]`) → _planned_

---

## Struct / Blueprint

```rupa
Person {
   name: string
   age: number
}

person: Person = { name: "Rupa", age: 20 }
person.name = "Rupa"
```

- `StructName {}` mendefinisikan tipe.
- Deklarasi variabel dengan tipe struct: `var: StructName = {...}`.
- Tidak ada `new Struct()` untuk saat ini.

---

## Function

```rupa
sayHello() {
  print("Hello Rupa")
}

add(x: number, y: number) {
  return x + y
}
```

- Fungsi cukup dengan `nameFn() {}`.
- Parameter bisa tanpa tipe (`x, y`) atau dengan tipe (`x: number`).
- Tipe bersifat dinamis dan strict.

---

## Import

```rupa
import sys from rupa
import sys, render, resources from rupa
import rupa.system as sys
```

- **Single Import:** `import mod from pkg`.
- **Multiple Import:** `import mod1, mod2 from pkg`.
- **Alias Import:** `import pkg.module as alias`.

---

<i>Catatan:</i>
Sintaks yang ditampilkan di sini merupakan **tahap awal**.
Detail validasi, error handling, dan fitur lanjutan akan ditambahkan setelah pengembangan ASTNode dan interpreter selesai.
