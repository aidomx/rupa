# Block Grammar

Grammar block membentuk node block dari statements.

## Simple block

Source:

```rupa
if true {
    print("inside block")
    print("still inside")
}
```

AST:

```text
Program:
  If:
    Condition: Boolean: true
    Body: Block
      Print: String: inside block
      Print: String: still inside
```

## Function body

Source:

```rupa
add(x, y) {
    result = x + y
    return result
}
```

AST:

```text
Program:
  Function:
    Name: Identifier: add
    Parameters:
      Identifier: x
      Identifier: y
    Body: Block
      Assignment:
        Target: Identifier: result
        Value: Binary: +
          Left: Identifier: x
          Right: Identifier: y
      Return: Identifier: result
```

## Nested blocks

Source:

```rupa
if x > 0 {
    if x > 10 {
        print("large")
    } else {
        print("small")
    }
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
      If:
        Condition: Binary: >
          Left: Identifier: x
          Right: Number: 10
        Body: Block
          Print: String: large
        Else: Block
          Print: String: small
```
