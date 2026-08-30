# Print

## Apa yang bisa ditulis?

```rupa
print("hello world")
```

Dengan multiple arguments:

```rupa
name = "rupa"
x = 1
y = 2

print(name)
print(name + " language")
print("x =", x)
print("result:", x + y, true)
```

Function call dan array:

```rupa
add(x, y) {
    return x + y
}

print(add(1, 2), [3, 4])
```

## Kapan digunakan?

Gunakan `print()` untuk menampilkan output ke terminal. Multiple arguments dipisahkan koma.

## Apa hasilnya?

```
hello world
rupa
rupa language
x = 1
result: 3 true
3 [3, 4]
```

Arguments dicetak berurutan tanpa separator. Tipe data ditampilkan sesuai representasinya.
