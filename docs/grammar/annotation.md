# Annotation Grammar

Annotation menghubungkan nama dengan type.

## Source

```rupa
name: string
age: number
price: float
```

AST:

```text
Annotation
├── Name
│   └── Identifier: name
└── Type
    └── Identifier: string
```

## Annotation dengan value

Source:

```rupa
x: number = 1
name: string = "Rupa"
```

Pada bentuk statement, parser membentuk annotation yang dapat membawa value.

Type yang sudah dikenali lexer juga dapat digunakan langsung oleh grammar lain,
terutama assignment dan parameter function.

## Context

Token `:` tidak selalu berarti hal yang sama di seluruh grammar. Test saat ini
menunjukkan context seperti:

```rupa
if x > 0: print(x)

x: number = 1

people: People = {
    name: "rupa",
    age: 20
}
```

Maka parser membedakan `:` berdasarkan struktur statement dan expression:

```text
if condition : body
name : Type = value
key : value
```

AST yang dihasilkan bergantung pada context tersebut.
