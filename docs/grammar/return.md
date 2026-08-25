# Return Grammar

Keyword `return` membentuk `NODE_RETURN` dengan satu expression child.

## Source

```rupa
return config
return x + y
```

AST:

```text
Return
└── Identifier: config

Return
└── Binary: +
    ├── x
    └── y
```

Grammar return memanggil grammar expression untuk bagian setelah keyword.
