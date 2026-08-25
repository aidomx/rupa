# If Grammar

Grammar if membentuk `NODE_IF`.

## Single-line body

```rupa
if x > 0: print(true)
```

AST:

```text
If
├── Condition
│   └── Binary: >
└── Body
    └── Block
        └── Print
```

## Block body

```rupa
if x > 0 {
    print(true)
}
```

Body tetap menjadi `Block`.

## Elseif dan else

```rupa
if x > 0 {
    print(true)
} elseif x == 0 {
    print("zero")
} else {
    print("negative")
}
```

AST aktual menunjukkan else branch dapat menyimpan `If` berikutnya:

```text
If
├── Condition: x > 0
├── Body: Block
└── Else
    └── If
        ├── Condition: x == 0
        ├── Body: Block
        └── Else
            └── Block
```

Dengan representasi ini, `elseif` dapat menjadi nested conditional dalam branch
`Else`, sedangkan `else` terakhir menyimpan block.
