# Update Grammar

Grammar update membentuk node update dari target dan operator.

## Postfix increment

Source:

```rupa
x++
```

AST:

```text
Update: postfix ++
  Identifier: x
```

## Postfix decrement

Source:

```rupa
x--
```

AST:

```text
Update: postfix --
  Identifier: x
```

## Compound assignment

Source:

```rupa
x += 5
```

AST:

```text
Assignment:
  Target: Identifier: x
  Value: Binary: +
    Left: Identifier: x
    Right: Number: 5
```

## Update in loop

Source:

```rupa
while x < 10 {
    print(x)
    x++
}
```

AST:

```text
Program:
  Loop: while
    Condition: Binary: <
      Left: Identifier: x
      Right: Number: 10
    Body: Block
      Print: Identifier: x
      Update: postfix ++
        Identifier: x
```
