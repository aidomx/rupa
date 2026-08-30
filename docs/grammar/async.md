# Async Grammar

Grammar async membentuk node async dari request, handler, dan timeout.

## Simple async

Source:

```rupa
async request => {
    print("Request started")
}
```

AST:

```text
Async:
  Request: Identifier: request
  Handler: Block
    Print: String: Request started
```

## Async with timeout

Source:

```rupa
async request => {
    print("Request started")
} timeout 5000 {
    print("Request timed out")
}
```

AST:

```text
Async:
  Request: Identifier: request
  Handler: Block
    Print: String: Request started
  Timeout: Number: 5000
  TimeoutHandler: Block
    Print: String: Request timed out
```

## Async structure

```text
Async
├── Request
│   └── Identifier: request
├── Handler
│   └── Block
├── Timeout
│   └── Number: 5000
└── TimeoutHandler
    └── Block
```
