# Call Grammar

Call expression membentuk `NODE_CALL`.

## Source

```rupa
add(1, 2)
```

AST:

```text
Call
├── Callee
│   └── Identifier: add
├── Arg 1
│   └── Number: 1
└── Arg 2
    └── Number: 2
```

## Call sebagai expression

Test print menunjukkan call dapat menjadi argument:

```rupa
print(add(1, 2), [3, 4])
```

AST:

```text
Print
├── Call
│   ├── Callee: add
│   ├── Arg 1: 1
│   └── Arg 2: 2
└── ArrayLiteral
    ├── 3
    └── 4
```

Grammar call dapat dipakai sebagai expression di dalam construct lain.
