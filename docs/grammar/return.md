# Return Grammar

Grammar return membentuk node return dari expression.

## Return with value

Source:

```rupa
return x + y
```

AST:

```text
Return:
  Binary: +
    Left: Identifier: x
    Right: Identifier: y
```

## Return in function

Source:

```rupa
add(x, y) {
    return x + y
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
      Return:
        Binary: +
          Left: Identifier: x
          Right: Identifier: y
```

## Return with object

Source:

```rupa
makePoint(px, py) {
    return { x: px, y: py }
}
```

AST:

```text
Program:
  Function:
    Name: Identifier: makePoint
    Parameters:
      Identifier: px
      Identifier: py
    Body: Block
      Return:
        Object:
          Entry 1:
            Key: Identifier: x
            Value: Identifier: px
          Entry 2:
            Key: Identifier: y
            Value: Identifier: py
```
