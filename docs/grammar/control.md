# Control Grammar

Grammar control membentuk node break dan continue.

## Break

Source:

```rupa
break
```

AST:

```text
Break
```

## Continue

Source:

```rupa
continue
```

AST:

```text
Continue
```

## Usage in loop

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
      Print: Identifier: i
```
