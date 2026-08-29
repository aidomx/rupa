# Math

Modul `math` menyediakan operasi matematika umum.

```rupa
import math from rupa
```

## Fungsi

```rupa
print(math.abs(-8))
print(math.sqrt(25))
print(math.pow(2, 3))
print(math.floor(3.8))
print(math.ceil(3.2))
print(math.round(3.5))
print(math.sin(0))
print(math.cos(0))
print(math.tan(0))
```

| Fungsi | Parameter | Deskripsi |
|---|---|---|
| `abs(value)` | number | Nilai absolut |
| `sqrt(value)` | number non-negatif | Akar kuadrat |
| `pow(base, exponent)` | dua number | Perpangkatan |
| `floor(value)` | number | Pembulatan ke bawah |
| `ceil(value)` | number | Pembulatan ke atas |
| `round(value)` | number | Pembulatan terdekat |
| `sin(value)` | number | Sinus, dalam radian |
| `cos(value)` | number | Cosinus, dalam radian |
| `tan(value)` | number | Tangen, dalam radian |

Fungsi matematika mengembalikan integer jika hasilnya merupakan integer yang dapat direpresentasikan, atau decimal jika tidak.
