# Expression Grammar

Grammar expression adalah pusat pembentukan value dan expression majemuk.

Dispatcher expression mencoba bentuk khusus terlebih dahulu:

```text
array literal
object literal
call expression
binary expression
```

Jika expression tidak cocok dengan bentuk literal khusus, parser binary membangun
struktur berdasarkan operator dan precedence.

## Binary expression

Source:

```rupa
x = 1 + 2
```

AST:

```text
Binary: +
├── Left
│   └── Number: 1
└── Right
    └── Number: 2
```

## Precedence

Test menunjukkan:

```rupa
x = 1 + 2 * 3
```

AST:

```text
Binary: +
├── Left: Number: 1
└── Right
    └── Binary: *
        ├── Left: Number: 2
        └── Right: Number: 3
```

Artinya `*` mengikat lebih kuat daripada `+`.

Parentheses mengubah struktur:

```rupa
x = ((1 + 2) * (3 - 4))
```

AST:

```text
Binary: *
├── Left
│   └── Binary: +
└── Right
    └── Binary: -
```

## Expression statement

Expression yang berdiri sendiri diparse sebagai expression statement dan
dibungkus menjadi `Return` node. Mekanisme ini dipakai parser untuk membawa hasil
expression, terutama sesuai kebutuhan REPL.

Contoh literal berdiri sendiri:

```rupa
1
3.14
true
null
"hello"
```

Pada test, bentuk tersebut menjadi `Return` dengan expression sebagai child.
