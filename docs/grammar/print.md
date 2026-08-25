# Print Grammar

`print(...)` membentuk `NODE_PRINT`.

Argument dipisahkan koma dan masing-masing diparse sebagai expression.

## Source

```rupa
print("result:", x + y, true)
```

AST:

```text
Print
├── String: "result:"
├── Binary: +
│   ├── x
│   └── y
└── Boolean: true
```

Grammar print tidak membatasi argument pada literal. Test saat ini menunjukkan
identifier, binary expression, call, dan array dapat menjadi argument.
