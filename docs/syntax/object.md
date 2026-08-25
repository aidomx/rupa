# Object

## Apa yang bisa ditulis?

```rupa
{
    name: "rupa",
    age: 20
}
```

Object typed:

```rupa
people: People = {
    name: "rupa",
    age: 20
}
```

## Kapan digunakan?

Gunakan object untuk mengelompokkan pasangan property dan value. Gunakan annotation sebelum `=` ketika object dikaitkan dengan type tertentu.

## Apa hasilnya?

Setiap entry memiliki key dan value. Value dapat berupa expression, bukan hanya literal. `:` di dalam object memisahkan key dan value, berbeda dari type annotation.
