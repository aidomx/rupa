# Print Grammar

Grammar print membentuk node print dari arguments yang dipisahkan koma.

## Single argument

Source:

```rupa
print("hello world")
```

AST:

```text
Program:
  Print:
    String: "hello world"
```

## Multiple arguments

Source:

```rupa
print("result:", x + y, true)
```

AST:

```text
Program:
  Print:
    String: "result:"
    Binary: +
      Identifier: x
      Identifier: y
    Boolean: true
```

## Print dengan expression

Source:

```rupa
print(add(1, 2), [3, 4])
```

AST:

```text
Program:
  Print:
    Call:
      Callee: Identifier: add
      Arg 1: Number: 1
      Arg 2: Number: 2
    Array:
      Number: 3
      Number: 4
```
