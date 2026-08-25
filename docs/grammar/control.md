# Control Grammar

Control grammar saat ini mencakup:

```rupa
break
continue
```

Keduanya tidak membawa expression.

AST:

```text
Break
Continue
```

Dalam test, node ini muncul di dalam `If` yang berada di body loop.
Grammar parser memastikan bentuk control tidak membawa token tambahan pada
statement yang sama.
