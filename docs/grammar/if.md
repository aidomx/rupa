# If Grammar

Grammar if membentuk node if dari condition, body, dan branch opsional.

## Simple if

Source:

```rupa
if x > 0 {
    print("positive")
}
```

AST:

```text
Program:
  If:
    Condition: Binary: >
      Left: Identifier: x
      Right: Number: 0
    Body: Block
      Print: String: positive
```

## If-else

Source:

```rupa
if x > 0 {
    print("positive")
} else {
    print("non-positive")
}
```

AST:

```text
Program:
  If:
    Condition: Binary: >
      Left: Identifier: x
      Right: Number: 0
    Body: Block
      Print: String: positive
    Else: Block
      Print: String: non-positive
```

## If-elseif-else

Source:

```rupa
if x > 0 {
    print("positive")
} elseif x == 0 {
    print("zero")
} else {
    print("negative")
}
```

AST:

```text
Program:
  If:
    Condition: Binary: >
      Left: Identifier: x
      Right: Number: 0
    Body: Block
      Print: String: positive
    Else: If
      Condition: Binary: ==
        Left: Identifier: x
        Right: Number: 0
      Body: Block
        Print: String: zero
      Else: Block
        Print: String: negative
```

## Inline if

Source:

```rupa
if x > 0: print(true)
```

AST:

```text
Program:
  If:
    Condition: Binary: >
      Left: Identifier: x
      Right: Number: 0
    Body: Print: Boolean: true
```
