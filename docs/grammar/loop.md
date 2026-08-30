# Loop Grammar

Grammar loop membentuk node loop dari kind, condition, dan body.

## For loop

Source:

```rupa
for i < 10 {
    print(i)
}
```

AST:

```text
Program:
  Loop: for
    Condition: Binary: <
      Left: Identifier: i
      Right: Number: 10
    Body: Block
      Print:
        Identifier: i
```

## Rev loop

Source:

```rupa
rev i > 0 {
    print(i)
}
```

AST:

```text
Program:
  Loop: rev
    Condition: Binary: >
      Left: Identifier: i
      Right: Number: 0
    Body: Block
      Print:
        Identifier: i
```

## While loop

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
      Print:
        Identifier: x
      Update: postfix ++
        Identifier: x
```

## Loop with break/continue

Source:

```rupa
for i < 10 {
    if i == 3: continue
    if i == 7: break
    print(i)
}
```

AST:

```text
Program:
  Loop: for
    Condition: Binary: <
      Left: Identifier: i
      Right: Number: 10
    Body: Block
      If:
        Condition: Binary: ==
          Left: Identifier: i
          Right: Number: 3
        Body: Continue
      If:
        Condition: Binary: ==
          Left: Identifier: i
          Right: Number: 7
        Body: Break
      Print:
        Identifier: i
```
