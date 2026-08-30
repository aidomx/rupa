# Call

## Apa yang bisa ditulis?

```rupa
add(1, 2)
print("hello")
greet()
```

Nested call:

```rupa
double(n) {
    return n * 2
}
triple(n) {
    return n * 3
}
val = double(triple(5))
```

## Kapan digunakan?

Gunakan call untuk menjalankan function yang sudah dideklarasikan. Arguments dipisahkan koma.

## Apa hasilnya?

Call mengevaluasi arguments dan menjalankan function body. Hasilnya adalah return value dari function.

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
print("hello world")
```

Output: `hello world`

```rupa
double(n) {
    return n * 2
}
triple(n) {
    return n * 3
}
val = double(triple(5))
print(val)
```

Output: `30`
