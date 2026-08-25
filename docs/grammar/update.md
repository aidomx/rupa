# Update Grammar

Update grammar saat ini membentuk `NODE_UPDATE`.

## Source

```rupa
x++
i--
```

AST test:

```text
Update: postfix ++
└── Identifier: x

Update: postfix --
└── Identifier: i
```

Node update menyimpan target, operator, dan informasi bentuk prefix/postfix.
Test saat ini secara eksplisit mencakup bentuk postfix.
