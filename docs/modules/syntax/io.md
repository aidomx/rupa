# IO

Modul `io` menyediakan input terminal dan konversi nilai dasar.

```rupa
import io from rupa
```

## Fungsi

### `io.input(prompt?)`

Membaca satu baris dari standard input. Prompt bersifat opsional dan hasilnya berupa string atau `null` saat input gagal/EOF.

```rupa
name = io.input("Name: ")
print(name)
```

### `io.toNumber(value)`

Mengubah string menjadi angka integer atau decimal. Jika nilai tidak dapat dikonversi, fungsi mengembalikan `null`.

```rupa
count = io.toNumber("42")
price = io.toNumber("19.95")
invalid = io.toNumber("rupa")

print(count)
print(price)
print(invalid)
```

## Ringkasan

| Fungsi | Parameter | Return |
|---|---|---|
| `input(prompt?)` | string opsional | string/null |
| `toNumber(value)` | string | number/decimal/null |
