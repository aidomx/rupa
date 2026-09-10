# Print Grammar

Grammar `print` membentuk node `Print` dari satu atau lebih expression yang dipisahkan oleh koma.

Setiap expression menjadi child langsung dari node `Print`. Koma tidak menghasilkan node AST tersendiri.

## Single argument

Source:

```rupa
print('hello world\n');
```

AST:

```text
Program:
  Print:
    String: "hello world\n"
```

Satu argument menghasilkan satu child pada node `Print`.

## Multiple arguments

Source:

```rupa
print('x =', x, '\n');
```

AST:

```text
Program:
  Print:
    String: "x ="
    Literal ID: x
    String: "\n"
```

Setiap argument dipertahankan sebagai node terpisah di dalam `Print`.

Dengan demikian, bentuk:

```rupa
print(a, b, c);
```

direpresentasikan sebagai:

```text
Print:
  <expression a>
  <expression b>
  <expression c>
```

## Argument berupa expression

Argument `print` tidak terbatas pada literal. Sebuah argument dapat berupa expression yang menghasilkan node AST sendiri.

Source:

```rupa
print(name + '\n');
```

AST:

```text
Program:
  Print:
    Binary: +
      Left:
        Literal ID: name
      Right:
        String: "\n"
```

Expression tersebut tetap menjadi satu argument `Print`, sedangkan struktur operasinya direpresentasikan oleh node `Binary`.

## Multiple arguments dengan expression

Source:

```rupa
print('result:', x + y, true);
```

AST:

```text
Program:
  Print:
    String: "result:"
    Binary: +
      Left:
        Literal ID: x
      Right:
        Literal ID: y
    Boolean: true
```

Koma memisahkan tiga argument:

1. `"result:"`
2. `x + y`
3. `true`

Operator `+` hanya berlaku di dalam argument kedua dan menghasilkan `Binary` node.

## Function call sebagai argument

Source:

```rupa
print(add(1, 2));
```

AST:

```text
Program:
  Print:
    Call:
      Callee:
        Identifier: add
      Arg 1:
        Number: 1
      Arg 2:
        Number: 2
```

Function call merupakan satu expression dan menjadi satu argument pada `Print`.

## Expression kompleks

Argument dapat berupa expression yang terdiri dari beberapa operasi.

Source:

```rupa
print('result:' + x + y + ' true\n');
```

AST:

```text
Program:
  Print:
    Binary: +
      Left:
        Binary: +
          Left:
            Binary: +
              Left:
                String: "result:"
              Right:
                Literal ID: x
          Right:
            Literal ID: y
      Right:
        String: " true\n"
```

Struktur `Binary` mengikuti pengelompokan expression yang dibentuk parser.

## Function call dalam expression

Function call juga dapat digunakan sebagai bagian dari expression argument.

Source:

```rupa
print(add(1, 2) + ' ' + [3, 4] + '\n');
```

AST:

```text
Program:
  Print:
    Binary: +
      Left:
        Binary: +
          Left:
            Binary: +
              Left:
                Call:
                  Callee:
                    Identifier: add
                  Arg 1:
                    Number: 1
                  Arg 2:
                    Number: 2
              Right:
                String: " "
          Right:
            Number: 3
      Right:
        String: "\n"
```

`Print` hanya mengetahui bahwa seluruh expression tersebut merupakan satu argument. Struktur operasi di dalamnya ditentukan oleh node expression seperti `Binary` dan `Call`.

## String interpolation

String literal dapat berisi interpolation untuk menampilkan nilai variabel atau hasil ekspresi.

### Variable interpolation: `{name}`

Source:

```rupa
print('Hello {name}\n');
```

AST:

```text
Program:
  Print:
    StringInterp:
      String: "Hello "
      Literal ID: name
      String: "\n"
```

### Expression interpolation: `{{expr}}`

Source:

```rupa
print('{{add(1,2)}}\n');
print('{{user.name}}');
print('{{x + y}}');
```

AST:

```text
Program:
  Print:
    StringInterp:
      Call:
        Callee: Identifier: add
        Arg 1: Number: 1
        Arg 2: Number: 2
      String: "\n"
  Print:
    StringInterp:
      Member:
        Object:
          Identifier: user
        Member:
          Identifier: name
  Print:
    StringInterp:
      Binary: +
        Left:
          Literal ID: x
        Right:
          Literal ID: y
```

### Mixed literal dan interpolation

Source:

```rupa
print('Hello {name}, you have {{count}} items\n');
```

AST:

```text
Program:
  Print:
    StringInterp:
      String: "Hello "
      Literal ID: name
      String: ", you have "
      Literal ID: count
      String: " items\n"
```

Node `StringInterp` berisi array parts yang ber alternating antara `String` (literal text) dan expression nodes (variable, call, binary, member, dll).

## Struktur umum

Secara konseptual, grammar `print` dapat dipandang sebagai:

```text
print
  └── expression (, expression)*
```

dan struktur AST-nya:

```text
Print
  ├── Expression
  ├── Expression
  └── Expression
```

Jumlah child `Print` mengikuti jumlah argument yang diberikan.

### Contoh

Source:

```rupa
print(a, b + c, add(1, 2));
```

Struktur AST:

```text
Program:
  Print:
    Literal ID: a
    Binary: +
      Left:
        Literal ID: b
      Right:
        Literal ID: c
    Call:
      Callee:
        Identifier: add
      Arg 1:
        Number: 1
      Arg 2:
        Number: 2
```

Dengan struktur tersebut, `print` berperan sebagai node statement yang menampung daftar expression, sementara grammar expression tetap bertanggung jawab membentuk struktur masing-masing argument.
