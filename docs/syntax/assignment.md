# Assignment

## Apa yang bisa ditulis?

```rupa
x = 1
name = "rupa"
x = 1 + 2
```

Typed assignment:

```rupa
count: number = 2
```

Conditional assignment:

```rupa
result ?= valid -> "Sukses"
result ?= valid -> primary | fallback | "Unavailable"
```

## Kapan digunakan?

Gunakan `=` untuk memberikan value biasa kepada target. Gunakan `:` setelah nama untuk type annotation. Gunakan `?=` ketika value dibentuk melalui expression conditional atau fallback.

## Apa hasilnya?

Target menerima expression di sebelah kanan. `?=` bukan bentuk lain dari `=`; ia membentuk conditional assignment dan dapat dikombinasikan dengan `->` dan `|`.

### Contoh execution

```rupa
x = 1
y = 2
print(x + y)
```

Output: `3`

```rupa
count: number = 42
print(count)
```

Output: `42`
