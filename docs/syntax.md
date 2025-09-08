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
