# Type Annotation

## Apa yang bisa ditulis?

```rupa
name: string
age: number
price: float

x: number = 1
people: People = {}
```

Annotation juga dapat digunakan pada parameter function:

```rupa
add(x: number, y: number) {}
```

## Kapan digunakan?

Gunakan annotation ketika sebuah nama perlu menyatakan type yang diharapkan, terutama untuk variable, field struct, object typed, atau parameter.

## Apa hasilnya?

Nama memiliki informasi type. Jika disertai `=`, nilai diberikan sekaligus. Type annotation tidak sama dengan `:` pada body `if`/loop atau pasangan key/value object; maknanya ditentukan oleh context.
