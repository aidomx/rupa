# Loop Grammar

Loop direpresentasikan oleh satu `NODE_LOOP` yang menyimpan `kind`, `condition`,
dan `body`.

## Bentuk umum

```text
Loop := "for" Expr Body
      | "rev" Expr Body
      | "while" Expr Body
```

`Body` dapat berupa statement setelah `:` atau block `{ ... }`.

## Arah `for` dan `rev`

Arah range merupakan bagian dari keyword:

```text
for -> maju
rev -> mundur
```

Operator yang diterima juga ketat:

```text
for -> <  <=
rev -> >  >=
```

Operator tidak digunakan untuk membalik arah loop.

## Shorthand identifier

Condition identifier tunggal memiliki arti khusus.

```rupa
i = 10
for i
```

Secara internal:

```text
bound = i
counter(i) = 0
while i < bound
```

Untuk `rev`:

```rupa
i = 10
rev i
```

Secara internal:

```text
bound = i
counter(i) = bound - 1
while i >= 0
```

Identifier yang sama sengaja dipakai sebagai sumber batas sebelum loop dan
sebagai penampung nilai iterasi selama loop.

## Binary condition

Binary condition menggunakan identifier kiri sebagai counter yang sudah ada.
Nilainya tidak diinisiasi ulang.

```rupa
i = 0
for i < 10
```

`for` memakai nilai `i` saat ini dan menaikkannya setiap iterasi.

```rupa
i = 10
rev i > 0
```

`rev` memakai nilai `i` saat ini, mengambil langkah mundur pertama, lalu
menurunkan counter sampai batas bawah range.

Contoh AST:

```text
Loop: for
  Binary: <
    Left:
      Literal ID: i
    Right:
      Number: 10
  Block:
    ...
```

## `while`

`while` tidak menggunakan semantik range `for` atau `rev`. Condition dievaluasi
setiap iterasi dan perubahan state harus dilakukan oleh body.

```rupa
while x < 10 {
    print(x)
    x++
}
```

## Control flow

`break` menghentikan loop dan `continue` melewati sisa body iterasi saat ini.
`return` dan error diteruskan keluar dari loop.
