# Function Grammar

Function declaration dan function call menggunakan pola awal yang sama:
identifier diikuti parentheses. Parser membedakan declaration ketika bentuk
tersebut diikuti body block.

## Declaration

Source:

```rupa
add(x: number, y: number) {
    return x + y
}
```

AST:

```text
Function
├── Name
│   └── Identifier: add
├── Parameters
│   ├── Annotation: x → number
│   └── Annotation: y → number
└── Body
    └── Block
        └── Return
            └── Binary: +
                ├── x
                └── y
```

## Typed parameter

```rupa
save(config: Settings) {
    return config
}
```

Parameter typed direpresentasikan sebagai `Annotation`.

## Call statement

```rupa
add(1, 2)
```

Jika pola identifier + arguments tidak membentuk declaration dengan body,
parser membuat `Call`.
