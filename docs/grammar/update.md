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
Update: postfix +=
  Identifier: x
  Number: 5
```

Node `NODE_UPDATE` membawa operand kanan pada field `value` untuk
compound (`+=`, `-=`, `*=`, `/=`, `%=`); untuk `++`/`--` field ini `-1`.
Interpreter mengevaluasi sebagai `x OP v` (semantik persis
`interpretBinary`), mesin IR menurunkannya menjadi `IR_BINARY` +
`IR_STORE` pada slot variable.

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
