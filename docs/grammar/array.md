# Array Grammar

Grammar array membentuk node array dari element yang dipisahkan koma.

## Empty array

Source:

```rupa
[]
```

AST:

```text
Program:
  ArrayLiteral: (empty)
```

## Simple array

Source:

```rupa
[1, 2, 3]
```

AST:

```text
Program:
  ArrayLiteral:
    Number: 1
    Number: 2
    Number: 3
```

## Array with expressions

Source:

```rupa
[1 + 2, 3 * 4]
```

AST:

```text
Program:
  ArrayLiteral:
    Binary: +
      Left: Number: 1
      Right: Number: 2
    Binary: *
      Left: Number: 3
      Right: Number: 4
```

## Nested array

Source:

```rupa
[1, [2, 3], 4]
```

AST:

```text
Program:
  ArrayLiteral:
    Number: 1
    ArrayLiteral:
      Number: 2
      Number: 3
    Number: 4
```


## Array type

Type array menggunakan `[]` sebagai postfix dari type element.

Source:

```rupa
x: number[] = [1, 2, 3]
empty: number[] = []
```

AST:

```text
Program:
  Annotation:
    Name:
      Identifier: x
    Type:
      ArrayType:
        Identifier: number
    Value:
      ArrayLiteral:
        Number: 1
        Number: 2
        Number: 3

  Annotation:
    Name:
      Identifier: empty
    Type:
      ArrayType:
        Identifier: number
    Value:
      ArrayLiteral: (empty)
```

Nested array menggunakan `ArrayType` secara rekursif:

```rupa
x: number[][] = [[1, 2], [3, 4]]
```

AST type:

```text
ArrayType:
  ArrayType:
    Identifier: number
```

Type array membatasi type setiap element.

```rupa
x: number[] = [1, "a"]
```

`"a"` tidak sesuai dengan `number`, sehingga assignment ditolak oleh type checking.
