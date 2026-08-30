# Then

## Apa yang bisa ditulis?

```rupa
result ?= valid -> "Sukses"
```

## Kapan digunakan?

Gunakan `->` dalam conditional assignment untuk menentukan value yang dihasilkan jika condition terpenuhi.

## Apa hasilnya?

`->` membentuk expression `NODE_THEN`. Value di sebelah kanan dihasilkan jika condition di sebelah kiri terpenuhi.

### Contoh execution

```rupa
valid = true
result ?= valid -> "Condition met"
print(result)
```

Output: `Condition met`

```rupa
valid = false
result ?= valid -> "Condition met"
print(result)
```

Output: `null`
