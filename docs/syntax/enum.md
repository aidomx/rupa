# Enum

## Apa yang bisa ditulis?

```rupa
enum Color {
  RED
  GREEN = 5
  BLUE
}

enum TokenType {
  IDENTIFIER: string = "id"
  ASSIGN = 0
}
```

## Kapan digunakan?

Gunakan enum untuk mendefinisikan sekumpulan konstanta bernama di bawah satu
namespace. Member tanpa nilai mendapat angka otomatis (auto-increment), member
dengan nilai eksplisit dipakai sebagai titik lanjutan counter.

## Apa hasilnya?

- Member tanpa nilai → konstanta numerik berurut (0, 1, 2, ...).
- Member dengan nilai → nilai eksplisit (number atau string); member berikutnya
  tanpa nilai melanjutkan dari angka sebelumnya + 1.
- Member bertipe (`NAME: Type = value`) → nilai dievaluasi sesuai tipenya.
- Nama enum sendiri ter-bind sebagai object berisi seluruh member, sehingga
  bisa dibaca lewat member access.

```rupa
enum Color {
  RED
  GREEN = 5
  BLUE
}

print(Color.RED)    # 0
print(Color.GREEN)  # 5
print(Color.BLUE)   # 6  (auto: 5 + 1)
```

Member juga bisa diakses langsung sebagai konstanta:

```rupa
enum Color { RED GREEN }

print(RED)  # 0
print(GREEN)  # 1
```

## EnumError — member tidak dikenal & proteksi konstanta

Member enum bersifat konstanta dan lookup-nya ketat:

```rupa
enum Color { RED GREEN }

print(Color.Green)  # EnumError: 'Green' is not a member of enum 'Color'
Color.RED = 9       # EnumError: cannot assign member 'RED' on enum 'Color'
```

Typo casing (mis. `Green` vs `GREEN`) ditolak dengan error eksplisit, bukan
`null` senyap. Ini berlaku di mode interpreter maupun mode IR (`rupa <file>`).
Berbeda dengan object biasa yang tetap mengembalikan `null` untuk member yang
tidak ada (backward compatible).
