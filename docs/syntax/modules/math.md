# Math

Modul `math` ditulis dengan bahasa Rupa dan tersedia melalui root namespace
`rupa`. Package ini dapat dikembangkan di `tests/modules/math/` untuk
simulasi stdlib project, lalu dipublikasikan ke archive module.

```rupa
import math from rupa
```

## Fungsi

```rupa
print(math.add(2, 3))
print(math.sub(10, 4))
print(math.mul(3, 7))
print(math.div(20, 4))
print(math.mod(17, 5))
print(math.clamp(15, 0, 10))
print(math.lerp(0, 100, 0.5))
print(math.isEven(4))
print(math.isOdd(7))
print(math.factorial(5))
```

| Fungsi | Parameter | Deskripsi |
|---|---|---|
| `add(a, b)` | dua number | Penjumlahan |
| `sub(a, b)` | dua number | Pengurangan |
| `mul(a, b)` | dua number | Perkalian |
| `div(a, b)` | dua number | Pembagian |
| `mod(a, b)` | dua number | Sisa pembagian |
| `clamp(value, min, max)` | tiga number | Membatasi nilai ke rentang |
| `lerp(a, b, t)` | tiga number | Interpolasi linear |
| `isEven(value)` | number | Memeriksa bilangan genap |
| `isOdd(value)` | number | Memeriksa bilangan ganjil |
| `factorial(value)` | number | Faktorial |

## System module

Implementasi `math` berasal dari source Rupa dan dikemas ke dalam binary
sebagai system module. Pengguna cukup memakai:

```rupa
import math from rupa

print(math.sqrt(25))
print(math.pow(2, 3))
```

Source pengembangan berada di `tests/modules/math/`. Saat build, source tersebut
dikemas menjadi `modules/rupa_modules.tar.gz` dan archive-nya ditanam ke binary.
