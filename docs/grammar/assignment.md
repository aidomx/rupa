# Assignment Grammar

Grammar assignment membentuk node assignment dari target dan expression nilai.

## Normal assignment

Source:

```rupa
name = "rupa"
x = 1 + 2
```

AST:

```text
Assignment
├── Target
│   └── Identifier: name
└── Value
    └── String: "rupa"

Assignment
├── Target
│   └── Identifier: x
└── Value
    └── Binary: +
        ├── Left: Number: 1
        └── Right: Number: 2
```

Node utama dibuat sebagai assignment biasa.

## Typed assignment

Source:

```rupa
count: number = 2

people: People = {
    name: "rupa",
    age: 20
}
```

AST:

```text
Assignment
├── Target
│   └── Identifier: count
├── Type
│   └── Identifier: number
└── Value
    └── Number: 2
```

Type annotation menjadi child `Type` pada assignment.

## Conditional assignment

Source:

```rupa
result ?= valid -> "Sukses"
```

AST aktual:

```text
Conditional Assignment
├── Target
│   └── Identifier: result
└── Value
    └── Then
        ├── Condition
        │   └── Literal ID: valid
        └── Result
            └── String: "Sukses"
```

`?=` membentuk `NODE_CONDITIONAL_ASSIGN`. Expression di sebelah kanan tidak
diparse sebagai grammar khusus conditional assignment; expression tersebut tetap
diparse melalui grammar expression.

Karena itu hasil `?=` dapat berupa `NODE_THEN`, `NODE_FALLBACK`, binary
expression, call, array, object, atau expression lain yang didukung parser.

## Komposisi dengan then dan fallback

Source:

```rupa
result ?= valid -> primary | fallback | "Unavailable"
```

AST aktual:

```text
Conditional Assignment
├── Target
│   └── Identifier: result
└── Value
    └── Then
        ├── Condition
        │   └── Literal ID: valid
        └── Result
            └── Fallback
                ├── Primary
                │   └── Fallback
                │       ├── Primary: Literal ID: primary
                │       └── Fallback: Literal ID: fallback
                └── Fallback: String: "Unavailable"
```

Ini menunjukkan tiga grammar berbeda:

```text
?=  → NODE_CONDITIONAL_ASSIGN
->  → NODE_THEN
|   → NODE_FALLBACK
```
