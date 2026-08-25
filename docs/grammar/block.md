# Block Grammar

Block menggunakan:

```rupa
{
    statement
    statement
}
```

Parser membaca setiap statement di dalam rentang brace dan membentuk satu
`NODE_BLOCK` yang menyimpan child statement.

## Keyword body

Keyword tertentu juga menerima body satu baris melalui `:`:

```rupa
if x > 0: print(x)
for i < 10: print(i)
```

Body setelah `:` diparse sebagai satu statement dan kemudian tetap dibungkus
menjadi `Block`.

Secara AST, bentuk satu baris dan brace body memiliki container `Block`:

```text
If / Loop
└── Body
    └── Block
        └── Statement
```

Hal ini menyatukan representasi AST meskipun syntax source berbeda.
