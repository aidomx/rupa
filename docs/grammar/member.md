# Member Grammar

Grammar member membentuk node member access dari object dan member.

## Simple member access

Source:

```rupa
person.name
```

AST:

```text
Member:
  Object: Identifier: person
  Member: Identifier: name
```

## Nested member access

Source:

```rupa
company.ceo.name
```

AST:

```text
Member:
  Object: Member:
    Object: Identifier: company
    Member: Identifier: ceo
  Member: Identifier: name
```

## Member with subscript

Source:

```rupa
team.members[0]
```

AST:

```text
Subscript:
  Base: Member:
    Object: Identifier: team
    Member: Identifier: members
  Index: Number: 0
```

## Usage

```rupa
person = { name: "Rupa", age: 1 }
print(person.name)
print(person.age)
```

Output:
```
Rupa
1
```

```rupa
company = { ceo: { name: "Boss", title: "CEO" } }
print(company.ceo.name)
```

Output: `Boss`
