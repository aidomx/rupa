# Case Grammar

Grammar case membentuk node case dari subject dan entries.

## Simple case

Source:

```rupa
case status => {
    200: print("success")
    404: print("not found")
    *: print("unknown")
}
```

AST:

```text
Program:
  Case:
    Subject: Identifier: status
    Entry:
      Pattern: Number: 200
      Body: Print: String: success
    Entry:
      Pattern: Number: 404
      Body: Print: String: not found
    Wildcard:
      Body: Print: String: unknown
```

## Case structure

```text
Case
├── Subject
│   └── Identifier: status
├── Entry
│   ├── Pattern: Number: 200
│   └── Body: Print
├── Entry
│   ├── Pattern: Number: 404
│   └── Body: Print
└── Wildcard
    └── Body: Print
```

Wildcard `*` menangkap semua case yang tidak terpenuhi.
