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
Program:
  Function:
    Name:
      Identifier: add
    Parameters:
      Annotation:
        Name: Identifier: x
        Type: Identifier: number
      Annotation:
        Name: Identifier: y
        Type: Identifier: number
    Body:
      Block:
        Return:
          Binary: +
            Identifier: x
            Identifier: y
```

## Call statement

```rupa
add(1, 2)
```

Jika pola identifier + arguments tidak membentuk declaration dengan body,
parser membuat `Call`:

```text
Program:
  Call:
    Callee:
      Identifier: add
    Arg 1:
      Number: 1
    Arg 2:
      Number: 2
```

## Function with object return

```rupa
makePoint(px, py) {
    return { x: px, y: py }
}
```

AST:

```text
Program:
  Function:
    Name:
      Identifier: makePoint
    Parameters:
      Identifier: px
      Identifier: py
    Body:
      Block:
        Return:
          Object:
            Entry 1:
              Key: Identifier: x
              Value: Identifier: px
            Entry 2:
              Key: Identifier: y
              Value: Identifier: py
```
