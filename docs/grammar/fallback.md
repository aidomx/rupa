# Fallback Chain Grammar

`|` membentuk `NODE_FALLBACK`.

## Source

```rupa
value = first | second
```

AST:

```text
Fallback
├── Primary
│   └── Literal ID: first
└── Fallback
    └── Literal ID: second
```

## Chain

Source:

```rupa
value = primary | fallback | default
```

AST aktual:

```text
Fallback
├── Primary
│   └── Fallback
│       ├── Primary: Literal ID: primary
│       └── Fallback: Literal ID: fallback
└── Fallback
    └── Literal ID: default
```

Jadi implementasi parser saat ini membentuk tree untuk chain tersebut; `|` tidak
disimpan sebagai array flat khusus.

## Digabung dengan then

Source:

```rupa
result ?= valid -> primary | fallback | "Unavailable"
```

Bagian kanan `Then` menjadi `Fallback` tree:

```text
Then
├── Condition: valid
└── Result
    └── Fallback
        ├── Primary: Fallback(primary, fallback)
        └── Fallback: "Unavailable"
```

`|` tidak bergantung pada `?=` atau `->`. Ketiganya adalah grammar terpisah
yang dapat dikomposisikan.
