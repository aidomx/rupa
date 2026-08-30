# Function

## Apa yang bisa ditulis?

```rupa
add(x: number, y: number) {
    return x + y
}
```

Pemanggilan:

```rupa
add(1, 2)
```

Function tanpa parameter:

```rupa
greet() {
    print("Hello!")
}
greet()
```

Function dengan object return:

```rupa
makePoint(px, py) {
    return { x: px, y: py }
}
p = makePoint(10, 20)
print(p.x)
```

## Kapan digunakan?

Gunakan declaration ketika ingin memberi nama pada logic yang dapat dipanggil kembali. Gunakan parameter annotation ketika parameter memiliki type yang diharapkan.

## Apa hasilnya?

Declaration memiliki nama, parameter, dan body. Identifier yang diikuti argument tetapi tidak membentuk declaration digunakan sebagai call.

### Contoh execution

```rupa
add(x: number, y: number) {
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
