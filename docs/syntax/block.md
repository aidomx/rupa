# Block

## Apa yang bisa ditulis?

```rupa
if x > 0 {
    print("positive")
    print("x is positive")
}
```

Function body:

```rupa
add(x, y) {
    result = x + y
    return result
}
```

## Kapan digunakan?

Gunakan block `{ }` untuk mengelompokkan beberapa statements. Block digunakan dalam function body, if/else, loop, dan construct lainnya.

## Apa hasilnya?

Block mengeksekusi semua statements di dalamnya secara berurutan. Variables dalam block memiliki scope sendiri.

### Contoh execution

```rupa
if true {
    print("inside block")
    print("still inside")
}
```

Output:
```
inside block
still inside
```

```rupa
add(x, y) {
    result = x + y
    return result
}

print(add(3, 7))
```

Output: `10`
