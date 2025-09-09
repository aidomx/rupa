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

AST Contoh:

```
Assignment:
  Target:
    Identifier: x
  Value:
    LiteralIdentifier: x
```

---

## Typed Assignment

```rupa
x: number = 1
```

- `: number` → deklarasi tipe variabel.
- Rupa mendukung tipe dasar: `null`, `true`, `false`, `number`, `string`, `float`.

---

## Expression

```rupa
x = (1+2)
x = ((1+2)*3)/((4+5)-6)
```

- Mendukung operasi aritmetika dasar:

  - Penjumlahan (`+`)
  - Pengurangan (`-`)
  - Perkalian (`*`)
  - Pembagian (`/`)
  - Tanda kurung `()` untuk prioritas operasi

---

## Subscript & Nested Access

```rupa
x[0] = 1
x[0][1] = 2
x[0][1][2] = 3
```

- `x[0] = 1` → assignment ke elemen tunggal.
- `x[0][1] = 2` → akses nested (array dalam array).
- `x[0][1][2] = 3` → nested lebih dalam.

AST Contoh:

```
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
    Number: 3
```

---

## Complex Subscript Expressions

```rupa
x[1+2][y] = ((1+2)*3)
```

- Index pertama (`[1+2]`) → expression aritmetika.
- Index kedua (`[y]`) → literal identifier.
- Nilai (`((1+2)*3)`) → expression kompleks.

AST Contoh:

```
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
        LiteralIdentifier: y
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
```

---

## Tipe Data yang Didukung

- `null`
- `true` / `false` (boolean)
- `number` (bilangan bulat)
- `float` (bilangan desimal)
- `string` (teks)

---

<i>Catatan:</i>
Sintaks yang ditampilkan di sini merupakan **tahap awal**.
Detail validasi, error handling, dan fitur lanjutan akan ditambahkan setelah pengembangan ASTNode dan interpreter selesai.
