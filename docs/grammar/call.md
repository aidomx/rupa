# Call Grammar

Grammar call membentuk node call dari callee dan arguments.

## Simple call

Source:

```rupa
add(1, 2)
```

AST:

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

## Nested call

Source:

```rupa
double(triple(5))
```

AST:

```text
Program:
  Call:
    Callee:
      Identifier: double
    Arg 1:
      Call:
        Callee:
          Identifier: triple
        Arg 1:
          Number: 5
```

## Call with expression args

Source:

```rupa
print("result:", x + y, true)
```

AST:

```text
Program:
  Call:
    Callee:
      Identifier: print
    Arg 1:
      String: result:
    Arg 2:
      Binary: +
        Left: Identifier: x
        Right: Identifier: y
    Arg 3:
      Boolean: true
```
