# Then Grammar

`->` adalah grammar expression tersendiri dan membentuk `NODE_THEN`.

## Source

```rupa
valid -> "Sukses"
```

AST:

```text
Then
├── Condition
│   └── Literal ID: valid
└── Result
    └── String: "Sukses"
```

Secara pembacaan:

> jika condition, maka result

Node ini dapat menjadi expression di dalam grammar lain.

Contoh:

```rupa
result ?= valid -> "Sukses"
```

AST tidak membuat operator gabungan tunggal. Struktur yang terbentuk adalah:

```text
Conditional Assignment
└── Value
    └── Then
        ├── Condition
        └── Result
```

Dengan demikian `->` tetap merupakan grammar `Then`, sedangkan `?=` tetap
merupakan grammar assignment.
