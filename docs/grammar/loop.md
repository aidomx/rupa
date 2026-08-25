# Loop Grammar

Grammar loop membentuk satu `NODE_LOOP` dengan kind, condition, dan body.

## Kinds

Test saat ini mencakup:

```rupa
for i: print(i)
rev i: print(i)
for i < 10: print(i)
rev i > 0: print(i)
while x < 10: print(x)
```

AST menyimpan kind seperti:

```text
Loop: for
Loop: rev
Loop: while
```

Condition dapat berupa identifier sederhana atau binary expression.

## Body

Body dapat berupa satu statement setelah `:` atau block:

```rupa
for i < 10 {
    print(i)
}
```

AST:

```text
Loop: for
├── Condition
│   └── Binary: <
└── Block
    └── ...
```

## Nested control

Test juga menunjukkan `break` dan `continue` berada sebagai child block:

```rupa
for i < 10 {
    if i == 1: continue
    if i == 8: break
    print(i)
}
```

## Update di dalam loop

```rupa
while x < 10 {
    print(x)
    x++
}
```

Update menjadi statement AST di dalam block loop.
