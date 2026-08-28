# Loop

Rupa memiliki tiga loop utama: `for`, `rev`, dan `while`.

## `for`

`for` selalu bergerak maju dan hanya menerima kondisi `<` atau `<=`.

### Bentuk singkat

```rupa
i = 10
for i: print(i)
```

Nilai `i` disalin sebagai batas, kemudian `i` menjadi penampung iterasi yang
mulai dari `0`.

Hasil:

```text
0
1
2
3
4
5
6
7
8
9
```

### Dengan kondisi

```rupa
i = 0
for i < 10: print(i)
```

Pada bentuk ini `i` sudah merupakan counter. Nilai awalnya tidak diinisiasi
ulang oleh `for`.

Karena itu:

```rupa
i = 10
for i < 10: print(i)
```

Tidak menghasilkan apa pun karena kondisi awal sudah salah.

## `rev`

`rev` selalu bergerak mundur dan hanya menerima kondisi `>` atau `>=`.

### Bentuk singkat

```rupa
i = 10
rev i: print(i)
```

Nilai `10` disalin sebagai batas. Kemudian `i` menjadi counter dan dimulai dari
`9` hingga `0`.

### Dengan kondisi

```rupa
i = 10
rev i > 0: print(i)
```

Nilai `i` berasal dari pengguna. `rev` mengambil langkah mundur pertama,
sehingga hasilnya `9` sampai `0`.

## `while`

`while` menggunakan kondisi biasa dan tidak mengatur arah maupun inisiasi
counter.

```rupa
x = 0
while x < 10 {
    print(x)
    x++
}
```

## Body

Loop dapat memakai satu statement:

```rupa
for i: print(i)
```

atau block:

```rupa
for i {
    print(i)
}
```

Body dapat menggunakan `break`, `continue`, dan operasi seperti `++` atau `--`.
