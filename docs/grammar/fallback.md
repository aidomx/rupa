# Fallback Grammar

Grammar fallback membentuk node fallback dari primary dan fallback values.

## Simple fallback

Source:

```rupa
result ?= valid -> "Success" | "Default"
```

AST:

```text
Program:
  Conditional Assignment:
    Target: Identifier: result
    Value: Then
      Condition: Literal ID: valid
      Result: Fallback
        Primary: String: Success
        Fallback: String: Default
```

## Chained fallback

Source:

```rupa
result ?= valid -> primary | fallback | "Unavailable"
```

AST:

```text
Program:
  Conditional Assignment:
    Target: Identifier: result
    Value: Then
      Condition: Literal ID: valid
      Result: Fallback
        Primary: Fallback
          Primary: Literal ID: primary
          Fallback: Literal ID: fallback
        Fallback: String: Unavailable
```

## Grammar operators

```text
?=  → NODE_CONDITIONAL_ASSIGN
->  → NODE_THEN
|   → NODE_FALLBACK
```

## Usage

```rupa
valid = true
result ?= valid -> "Success"
print(result)
```

Output: `Success`

```rupa
valid = false
result ?= valid -> "Success" | "Default"
print(result)
```

Output: `Default`
