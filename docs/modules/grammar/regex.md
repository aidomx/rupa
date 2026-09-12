# Regex Module Grammar

## AST Structure

### `regex.match(pattern, str)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: regex
      Member:
        Identifier: match
  Arg 1:
    String: "[0-9]+"
  Arg 2:
    Identifier: str
```

### `regex.findAll(pattern, str)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: regex
      Member:
        Identifier: findAll
  Arg 1:
    Identifier: pattern
  Arg 2:
    Identifier: str
```

## Module Structure

```text
regex
├── match(pattern, str)      — test match
├── find(pattern, str)       — first match
├── findAll(pattern, str)    — all matches
├── replace(pattern, str, r, global?) — replace
└── split(pattern, str)      — split by pattern
```
