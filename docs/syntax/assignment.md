# Assignment

## Apa yang bisa ditulis?

```rupa
x = 1
name = "rupa"
x = 1 + 2

count: number = 2
```

Conditional assignment:

```rupa
result ?= valid -> "Sukses"
result ?= valid -> primary | fallback | "Unavailable"
```

## Kapan digunakan?

Gunakan `=` untuk memberikan value biasa kepada target. Gunakan `?=` ketika value dibentuk melalui expression conditional atau fallback.

## Apa hasilnya?

Target menerima expression di sebelah kanan. `?=` bukan bentuk lain dari `=`; ia membentuk conditional assignment dan dapat dikombinasikan dengan `->` dan `|`.
