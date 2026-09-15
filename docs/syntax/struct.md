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

### Validasi type (analyzer)

Deklarasi struct berlaku sebagai kontrak type. Assignment / annotation yang
memakai nama struct divalidasi terhadap layout-nya:

```rupa
Point {
  x: number
  y: number
}

p: Point = { x: 10, y: 20 }   # OK
q: Point = { x: 10, y: "a" }  # TypeError: expected 'number', got 'string'
r: Nope = {}                  # TypeError: type 'Nope' tidak dikenal
```

Validasi bersifat rekursif (struct dalam struct, array of struct), dan field
struct dengan type yang tidak dikenal ditolak saat deklarasi. Tidak ada
forward reference antar struct — deklarasikan dulu sebelum dipakai.

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
