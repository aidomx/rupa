# Object Grammar

Grammar object membentuk node object dari key-value pairs.

## Simple object

Source:

```rupa
person = { name: "Rupa", age: 1 }
```

AST:

```text
Program:
  Assignment:
    Target: Identifier: person
    Value:
      Object:
        Entry 1:
          Key: Identifier: name
          Value: String: Rupa
        Entry 2:
          Key: Identifier: age
          Value: Number: 1
```

## Nested object

Source:

```rupa
company = { ceo: { name: "Boss", title: "CEO" } }
```

AST:

```text
Program:
  Assignment:
    Target: Identifier: company
    Value:
      Object:
        Entry 1:
          Key: Identifier: ceo
          Value:
            Object:
              Entry 1:
                Key: Identifier: name
                Value: String: Boss
              Entry 2:
                Key: Identifier: title
                Value: String: CEO
```

## Object with array

Source:

```rupa
team = { members: ["Alice", "Bob", "Charlie"] }
```

AST:

```text
Program:
  Assignment:
    Target: Identifier: team
    Value:
      Object:
        Entry 1:
          Key: Identifier: members
          Value:
            Array:
              String: Alice
              String: Bob
              String: Charlie
```
