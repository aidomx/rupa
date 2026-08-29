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

## Member method sebagai call

Member method menggunakan `NODE_MEMBER` sebagai callee dari `NODE_CALL`:

```rupa
name.upper()
name.contains("R")
```

Receiver tidak ditulis sebagai argument eksplisit. Runtime native method menyimpan receiver dan meneruskannya sebelum argument call lainnya.

Detail aturan member tersedia di [member.md](member.md).

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
