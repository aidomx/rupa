# Annotation Grammar

Grammar annotation membentuk node annotation dari nama dan type.

## Simple annotation

Source:

```rupa
name: string
```

AST:

```text
Program:
  Annotation:
    Name:
      Identifier: name
    Type:
      Identifier: string
```

## Annotation with value

Source:

```rupa
x: number = 1
```

AST:

```text
Program:
  Annotation:
    Name:
      Identifier: x
    Type:
      Identifier: number
    Value:
      Number: 1
```

## Multiple annotations

Source:

```rupa
count: number = 42
name: string = "hello"
active: boolean = true
```

AST:

```text
Program:
  Annotation:
    Name: Identifier: count
    Type: Identifier: number
    Value: Number: 42
  Annotation:
    Name: Identifier: name
    Type: Identifier: string
    Value: String: hello
  Annotation:
    Name: Identifier: active
    Type: Identifier: boolean
    Value: Boolean: true
```
