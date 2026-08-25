# Grammar Async

## Definisi

`async` menciptakan konteks evaluasi asinkron untuk sebuah ekspresi permintaan (request expression).

Hasil dari sebuah `async-expression` adalah sebuah async handle.

## Bentuk Grammar

```text
async-expression
    = "async" request-expression
      [ async-handler ]
      [ timeout ]
```

## Komponen

### Request

```text
request-expression
```

Ekspresi yang dievaluasi secara asinkron.

## Handler

```text
async-handler
    = "=>" expression
    | "=>" block
```

## Timeout

```text
timeout
    = "->" expression
```

## Struktur Parse

```text
AsyncExpression
├── request
├── handler
└── timeout
```

## Hasil

```text
AsyncHandle
├── status
├── data
└── error
```

## Evaluasi

```text
AsyncExpression
        │
        ├── evaluate request asynchronously
        ├── create AsyncHandle
        ├── status = AWAIT
        ├── register handler
        └── apply timeout
```

## Ketergantungan Await

```text
await-expression
    = "await" expression
```

`await` menandai ekspresi yang memuatnya sebagai bergantung pada sebuah nilai asinkron.

## Konteks Handler

Di dalam sebuah async handler:

```text
this
└── AsyncHandle
    ├── status
    ├── data
    └── error
```

## Transisi State

```text
AWAIT
├── completed → SUCCESS
├── failed    → FAILED
└── timeout   → FAILED
```
