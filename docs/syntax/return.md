# Return

## Apa yang bisa ditulis?

```rupa
add(x, y) {
    return x + y
}
```

Return tanpa value:

```rupa
greet() {
    print("Hello!")
    return
}
```

## Kapan digunakan?

Gunakan `return` untuk mengembalikan value dari function. Return dapat digunakan di mana saja dalam function body.

## Apa hasilnya?

Return menghentikan eksekusi function dan mengembalikan value ke pemanggil.

### Contoh execution

```rupa
add(x, y) {
    return x + y
}

result = add(1, 2)
print(result)
```

Output: `3`

```rupa
double(n) {
    return n * 2
}

val = double(5)
print(val)
```

Output: `10`

```rupa
makePoint(px, py) {
    return { x: px, y: py }
}

p = makePoint(10, 20)
print(p.x, p.y)
```

Output: `10 20`
