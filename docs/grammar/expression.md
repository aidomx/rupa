# Expression Grammar

Grammar expression membentuk node expression dari kombinasi value dan operator.

## Binary expression

Source:

```rupa
1 + 2
```

AST:

```text
Program:
  Binary: +
    Left: Number: 1
    Right: Number: 2
```

## Mixed operations

Source:

```rupa
2 + 3 * 4
```

AST:

```text
Program:
  Binary: +
    Left: Number: 2
    Right: Binary: *
      Left: Number: 3
      Right: Number: 4
```

## String expression

Source:

```rupa
"hello" + " " + "world"
```

AST:

```text
Program:
  Binary: +
    Left: Binary: +
      Left: String: hello
      Right: String: " "
    Right: String: world
```
