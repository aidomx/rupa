# Struct Grammar

Grammar struct membentuk node struct dari nama dan field declarations.

## Simple struct

Source:

```rupa
People {
    name: string
    age: number
}
```

AST:

```text
Program:
  Struct:
    Name: Identifier: People
    Body:
      Block:
        Annotation:
          Name: Identifier: name
          Type: Identifier: string
        Annotation:
          Name: Identifier: age
          Type: Identifier: number
```

## Struct usage

Struct berfungsi sebagai blueprint untuk membuat object:

```rupa
People {
    name: string
    age: number
}

person = { name: "Rupa", age: 20 }
```

AST:

```text
Program:
  Struct:
    Name: Identifier: People
    Body:
      Block:
        Annotation:
          Name: Identifier: name
          Type: Identifier: string
        Annotation:
          Name: Identifier: age
          Type: Identifier: number
  Assignment:
    Target: Identifier: person
    Value:
      Object:
        Entry 1:
          Key: Identifier: name
          Value: String: Rupa
        Entry 2:
          Key: Identifier: age
          Value: Number: 20
```
