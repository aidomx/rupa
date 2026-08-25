# Object Grammar

Object literal menggunakan `{ ... }` dan diparse sebagai expression.

## Source

```rupa
people: People = {
    name: "rupa",
    age: 20
}
```

AST:

```text
Object
├── Entry 1
│   ├── Key
│   │   └── Identifier: name
│   └── Value
│       └── String: "rupa"
└── Entry 2
    ├── Key
    │   └── Identifier: age
    └── Value
        └── Number: 20
```

Setiap entry disimpan sebagai pasangan key dan value.

Value diparse sebagai expression, sehingga secara struktur dapat berkembang
menjadi expression lain yang didukung parser.
