# Annotation

## Apa yang bisa ditulis?

```rupa
name: string
age: number
price: float
values: number[]
```

Dengan value:

```rupa
x: number = 1
name: string = "Rupa"
values: number[] = [1, 2, 3]
empty: number[] = []
```

## Kapan digunakan?

Gunakan annotation untuk mendeklarasikan variable dengan type tertentu. Type annotation bersifat opsional.

## Apa hasilnya?

Annotation membuat binding dengan type information. Jika tidak ada value, hanya type yang dideklarasikan.

### Contoh execution

```rupa
count: number = 42
print(count)
```

Output: `42`

```rupa
name: string = "hello"
print(name)
```

Output: `hello`

```rupa
active: boolean = true
print(active)
```

Output: `true`

```rupa
pi: decimal = 3.14
print(pi)
```

Output: `3.14`

### Type annotation without value

```rupa
x: number
y: string
```

Ini hanya mendeklarasikan type tanpa value.


### Array type annotation

Gunakan `[]` setelah type untuk menyatakan array dengan element bertype tersebut.

```rupa
numbers: number[] = [1, 2, 3]
names: string[] = ["Rupa", "C"]
empty: number[] = []
```

Nested array menggunakan lebih dari satu `[]`:

```rupa
matrix: number[][] = [[1, 2], [3, 4]]
```

Array kosong tetap dapat diberi tipe; karena tidak memiliki element, validasi element tidak menghasilkan error:

```rupa
empty: number[] = []
```

Element yang tidak sesuai type akan menghasilkan error:

```rupa
numbers: number[] = [1, "a"] // error
```
