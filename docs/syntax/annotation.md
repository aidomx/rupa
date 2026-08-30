# Annotation

## Apa yang bisa ditulis?

```rupa
name: string
age: number
price: float
```

Dengan value:

```rupa
x: number = 1
name: string = "Rupa"
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
