# Literal Grammar

Grammar literal membentuk node literal dari nilai konstan.

## Number

Source:

```rupa
42
```

AST:

```text
Program:
  Number: 42
```

## Decimal

Source:

```rupa
3.14
```

AST:

```text
Program:
  Decimal: 3.14
```

## String

Source:

```rupa
"hello"
```

AST:

```text
Program:
  String: hello
```

## Boolean

Source:

```rupa
true
```

AST:

```text
Program:
  Boolean: true
```

## Null

Source:

```rupa
null
```

AST:

```text
Program:
  Nullable: null
```
