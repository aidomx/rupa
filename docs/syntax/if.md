# If

## Apa yang bisa ditulis?

```rupa
if x > 0: print(true)
```

Dengan block dan branch:

```rupa
if x > 0 {
    print(true)
} elseif x == 0 {
    print("zero")
} else {
    print("negative")
}
```

## Kapan digunakan?

Gunakan `if` untuk menjalankan body berdasarkan condition. Gunakan `elseif` untuk condition lanjutan dan `else` untuk kondisi selain branch sebelumnya.

## Apa hasilnya?

Hanya body dari branch yang sesuai condition yang dipilih. Body dapat berupa satu statement setelah `:` atau block `{ ... }`.
