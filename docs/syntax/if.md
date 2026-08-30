# If

## Apa yang bisa ditulis?

```rupa
if x > 0: print(true)
```

Dengan block dan branch:

```rupa
if x > 0 {
    print("positive")
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

### Contoh execution

```rupa
x = 10

if x > 0 {
    print("positive")
} elseif x == 0 {
    print("zero")
} else {
    print("negative")
}
```

Output: `positive`

```rupa
status = 404

case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    *: print("unknown")
}
```

Output: `not found`
