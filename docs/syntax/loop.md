# Loop

## Apa yang bisa ditulis?

Iterasi maju:

```rupa
for i: print(i)
for i < 10: print(i)
```

Iterasi mundur:

```rupa
rev i: print(i)
rev i > 0: print(i)
```

Condition pengguna:

```rupa
while x < 10 {
    print(x)
    x++
}
```

## Kapan digunakan?

Gunakan `for` untuk iterasi maju, `rev` untuk iterasi mundur, dan `while` ketika condition serta perubahan nilainya ditentukan sendiri.

## Apa hasilnya?

Loop menjalankan body berulang sesuai kind dan condition. Body dapat berisi `break`, `continue`, atau update seperti `x++`.
