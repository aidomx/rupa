# Rupa Syntax

Dokumen ini menjelaskan bentuk umum penulisan syntax dalam Rupa.

Setiap syntax dibahas lebih rinci pada dokumentasi masing-masing di [index.md](index.md).

## Bentuk dasar

Program Rupa dibentuk dari literal, expression, statement, dan block.

```rupa
name = "rupa"

if name == "rupa":
    print("Hello")
```

Syntax yang sama dapat digunakan dalam context yang berbeda selama bentuk dan context-nya valid.

## Statement dan block

Beberapa construct dapat menggunakan satu statement:

```rupa
if x > 0: print(true)
```

Atau block untuk beberapa statement:

```rupa
if x > 0 {
    print(true)
    print("positive")
}
```

Block dapat digunakan secara nested.

## Expression

Expression digunakan untuk menghasilkan, mengakses, atau menggabungkan nilai.

```rupa
x + y
user.name
users[i]
add(1, 2)
```

Expression dapat digabungkan:

```rupa
db.getUser().data
users[i].name
```

## Context

Beberapa tanda memiliki arti berdasarkan context.

Contohnya `:`:

```rupa
if x > 0: print(x)        // body satu statement
x: number = 1             // type annotation
name: "rupa"              // property/value
```

Lexer dan processor harus menentukan makna berdasarkan construct dan context aktif.

## Delimiter

Rupa menggunakan delimiter untuk membentuk dan membatasi syntax:

```text
()  argument, parameter, grouping
[]  array dan access
{}  block dan object
```

Delimiter dapat nested:

```rupa
print(add(1, 2), [3, 4])
```

## Type annotation

Type dapat ditulis setelah `:`:

```rupa
name: string
age: number

x: number = 1
price: float = 1.0
```

Type annotation juga dapat digunakan pada parameter:

```rupa
add(x: number, y: number) {
    return x + y
}
```

## Literal

Literal dasar meliputi:

```text
number
decimal
string
true
false
null
```

Decimal literal tidak menentukan `float` atau `double` saat lexing:

```rupa
x: float = 1.0
y: double = 1.0
z = 1.0
```

Precision ditentukan kemudian oleh type annotation atau inference.

## Pipeline

Secara umum, input melewati pipeline:

```text
input → lexeme → lexer → processor → token → parser/AST
```

Keyword dan construct menggunakan pipeline lexer yang sama. Processor dapat dipisah menjadi beberapa handler, tetapi tetap berbagi context.

## Dokumentasi syntax

Daftar seluruh syntax tersedia di [index.md](index.md).

Dokumentasi grammar di `docs/grammar/` menjelaskan bentuk syntax dari sisi sistem dan implementasi.
