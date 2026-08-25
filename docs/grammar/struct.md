# Struct Grammar

Struct declaration menggunakan identifier diikuti block.

## Source

```rupa
People {
    name: string
    age: number
}
```

AST:

```text
Struct
├── Identifier: People
└── Block
    ├── Annotation
    │   ├── Name: name
    │   └── Type: string
    └── Annotation
        ├── Name: age
        └── Type: number
```

Isi struct diparse sebagai block biasa. Dengan demikian annotation field menjadi
statement child di dalam block struct.
