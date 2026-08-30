# Struct

## Apa yang bisa ditulis?

```rupa
People {
    name: string
    age: number
}
```

## Kapan digunakan?

Gunakan struct untuk mendefinisikan tipe data dengan field yang telah ditentukan. Struct berfungsi sebagai blueprint untuk membuat object.

## Apa hasilnya?

Struct dibuat dengan nama diikuti block `{ }` yang berisi field declarations. Setiap field memiliki nama dan type annotation.

### Contoh execution

```rupa
People {
    name: string
    age: number
}

person = { name: "Rupa", age: 20 }
print(person.name)
print(person.age)
```

Output:
```
Rupa
20
```

```rupa
Point {
    x: number
    y: number
}

p = { x: 10, y: 20 }
print(p.x, p.y)
```

Output:
```
10 20
```
