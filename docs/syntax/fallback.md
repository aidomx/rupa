# Fallback

## Apa yang bisa ditulis?

```rupa
result ?= valid -> "Sukses"
result ?= valid -> primary | fallback | "Unavailable"
```

## Kapan digunakan?

Gunakan `?=` untuk conditional assignment. `->` menentukan condition, `|` menentukan fallback values.

## Apa hasilnya?

`?=` membentuk conditional assignment. Value dipilih berdasarkan condition dan fallback chain.

### Contoh execution

```rupa
valid = true
result ?= valid -> "Success"
print(result)
```

Output: `Success`

```rupa
valid = false
result ?= valid -> "Success" | "Default"
print(result)
```

Output: `Default`
