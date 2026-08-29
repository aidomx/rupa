# String Methods

String dapat mengakses property dan method langsung melalui member access.

```rupa
name = "Rupa"

print(name.length)
print(name.upper())
print(name.lower())
```

Hasilnya:

```text
4
RUPA
rupa
```

## Property `length`

`length` adalah property read-only yang menghasilkan jumlah karakter string:

```rupa
name = "Rupa"
print(name.length)
```

`name.length` setara secara konsep dengan pemanggilan fungsi string yang menerima `name` sebagai receiver, tetapi tidak menggunakan tanda kurung.

## Method string

Method yang tersedia:

```rupa
text.upper()
text.lower()
text.trim()
text.contains(search)
text.startsWith(prefix)
text.endsWith(suffix)
text.replace(old, new)
```

Contoh:

```rupa
text = "  Rupa Language  "

print(text.upper())
print(text.lower())
print(text.trim())
print(text.contains("Language"))
print(text.startsWith("  Ru"))
print(text.endsWith("  "))
print(text.replace("Language", "Lang"))
```

## Receiver method

Pada pemanggilan:

```rupa
name.upper()
```

`name` adalah receiver. Runtime meneruskan receiver tersebut secara otomatis kepada implementasi native method. Secara konsep, bentuk ini setara dengan:

```rupa
string.upper(name)
```

Namun bentuk member access lebih ringkas dan natural.

Module `string` tetap dapat digunakan untuk pemanggilan eksplisit:

```rupa
import string from rupa

print(string.upper("rupa"))
```

Kedua bentuk tersebut dapat didukung secara bersamaan.

## Chaining

Karena method transformasi menghasilkan string baru, method dapat dirangkai:

```rupa
name = "  Rupa  "
result = name.trim().upper()
print(result)
```

String awal tidak diubah. Method seperti `upper`, `lower`, `trim`, dan `replace` menghasilkan nilai string baru.

## Read-only property

`length` digunakan untuk membaca informasi string:

```rupa
name = "Rupa"
print(name.length)
```

Ia bukan method dan tidak dipanggil dengan `()`:

```rupa
# Bentuk yang benar
name.length

# Bukan bentuk property
# name.length()
```
