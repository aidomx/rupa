# Then Grammar

Grammar then membentuk node then dari condition dan result.

## Simple then

Source:

```rupa
result ?= valid -> "Sukses"
```

AST:

```text
Then:
  Condition: Literal ID: valid
  Result: String: Sukses
```

## Then with expression

Source:

```rupa
result ?= x > 0 -> "positive"
```

AST:

```text
Then:
  Condition: Binary: >
    Left: Identifier: x
    Right: Number: 0
  Result: String: positive
```

## Usage

```rupa
valid = true
result ?= valid -> "Condition met"
print(result)
```

Output: `Condition met`

```rupa
valid = false
result ?= valid -> "Condition met"
print(result)
```

Output: `null`
