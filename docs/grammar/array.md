# Array Grammar

Array literal diparse oleh grammar array ketika seluruh rentang expression
dibuka oleh `[` dan ditutup oleh `]`.

## Empty array

```rupa
[]
```

Membentuk `ArrayLiteral` tanpa elemen.

## Elements

```rupa
[1, 2, 3]
```

Setiap elemen diparse sebagai expression.

## Nested expression

```rupa
[1 + 2, 3 * 4]
[1, [2, 3], 4]
```

AST dapat berisi binary expression dan array lain sebagai child.

## Array of objects

```rupa
items = [
    { id: 1, name: "a" },
    { id: 2, name: "b" }
]
```

AST aktual:

```text
Assignment
└── Value
    └── ArrayLiteral
        ├── Object
        │   ├── Entry: id → 1
        │   └── Entry: name → "a"
        └── Object
            ├── Entry: id → 2
            └── Entry: name → "b"
```

Array adalah container AST; setiap elemen tetap dibentuk oleh grammar expression.
