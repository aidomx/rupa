# Rupa Syntax Reference

Dokumen ini adalah **acuan syntax Rupa yang sedang dikembangkan**. Fokus implementasi saat ini adalah lexer dan processor yang mengenali keyword, construct, delimiter, annotation, block, dan state input. Parser/AST tidak boleh diasumsikan sudah mendukung seluruh bentuk di bawah ini sampai implementasinya selesai.

## 1. Prinsip utama

Keyword dan construct melewati pipeline lexer yang sama.

```text
input → lexeme → lexer → processor → token → parser/AST
```

Processor dapat memecah handler berdasarkan tanggung jawab, tetapi handler harus tetap berbagi context.

## 2. Keyword

Keyword yang menjadi bagian dari desain saat ini:

```text
if
elseif
else
for
rev
while
print
return
import
export
extends
```

`write` tidak digunakan sebagai keyword output terpisah; gunakan `print`.

## 3. `if`, `elseif`, `else`

### Body satu statement

```rupa
if x > 0: print(true)
```

Bentuk input bertahap juga mewakili satu body statement:

```rupa
if x > 0:
print(true)
```

Setelah satu statement body selesai, context `if` harus selesai dan tidak boleh terbawa ke construct berikutnya.

### Block

```rupa
if x > 0 {
    print(true)
} elseif x == 0 {
    print("zero")
} else {
    print("negative")
}
```

`{ ... }` dapat berisi lebih dari satu statement dan dapat nested.

## 4. Loop

Rupa membedakan dua model loop:

- `for` dan `rev` memiliki arah iterasi bawaan.
- `while` menggunakan kondisi yang sepenuhnya ditentukan pengguna.

### `for` — maju

Bentuk paling sederhana menggunakan nilai sebagai batas. Sistem membuat iterator lokal yang bergerak maju dari `0`, tanpa mengubah nilai sumber di luar loop.

```rupa
i = 10
for i: print(i)
```

Secara konsep menghasilkan `0` sampai `9`, sementara setelah loop nilai `i` sumber tetap `10`.

`for` juga dapat menerima kondisi eksplisit:

```rupa
for i < 10: print(i)

for i < 10 {
    print(i)
}
```

### `rev` — mundur

`rev` adalah pasangan reverse dari `for`. Tanpa kondisi eksplisit, nilai sumber menjadi dasar iterasi dan iterator lokal bergerak menuju `0`. Nilai sumber tetap tidak berubah setelah loop.

```rupa
i = 10
rev i: print(i)
```

Kondisi eksplisit juga dapat digunakan ketika pengguna membutuhkan batas sendiri:

```rupa
rev i > 0: print(i)

rev i >= 0 {
    print(i)
}
```

### `while` — kondisi pengguna

`while` tidak memiliki arah atau iterator bawaan. Kondisi dan perubahan nilainya ditentukan pengguna.

```rupa
while x < 10: print(x)

while x < 10 {
    print(x)
    x++
}
```

Untuk `for` dan `rev`, implementasi runtime boleh memakai mekanisme evaluasi kondisi yang sama dengan `while`, tetapi secara grammar dan semantik keyword tetap menyatakan arah iterasi: `for` maju dan `rev` mundur.

## 5. Function/construct declaration

Bentuk declaration menggunakan identifier, parameter, annotation, dan block:

```rupa
add(x: number, y: number) {
    return x + y
}
```

Contoh pemanggilan:

```rupa
add(1, 2)
```

## 6. Struct/blueprint

Struct tidak memerlukan keyword `struct`. Nama construct langsung diikuti block:

```rupa
People {
    name: string
    age: number
}
```

Di dalam declaration seperti ini:

```rupa
name: string
```

`:` berarti type annotation.

## 7. Object typed

Object yang mengikuti struct/blueprint dapat ditulis:

```rupa
people: People = {
    name: "rupa",
    age: 20
}
```

Makna `:` di sini berbeda:

```rupa
people: People
        ↑ annotation type

name: "rupa"
    ↑ property/value separator
```

Context menentukan makna `:`. Identifier property tidak perlu menggunakan key string sebagai syntax utama Rupa.

## 8. Type annotation

```rupa
name: string
age: number
price: float

x: number = 1
name: string = "Rupa"
```

Parameter juga dapat diberi annotation:

```rupa
add(x: number) {
    return x
}
```

Nama tipe bukan statement keyword. Mereka diproses sebagai bagian dari type construct setelah `:`.

Tipe/literal dasar yang pernah dibahas:

```text
number
float
string
null
true
false
```

## 9. Assignment

```rupa
x = 1
x = y
name = "rupa"

x: number = 1
```

Question assignment:

```rupa
x ?= value
```

`?=` adalah operator/construct tersendiri dan bukan pengganti `=` biasa.

## 10. Literal

### Number

```rupa
0
1
10
100
```

### Float

```rupa
1.0
3.14
0.5
```

### Boolean

```rupa
true
false
```

### Null

```rupa
null
```

### String

```rupa
"hello"
'hello'
```

## 11. Expression dan operator

Aritmetika:

```text
+  -  *  /  %
```

Perbandingan:

```text
==  !=  <  <=  >  >=
```

Logical/operator lain yang telah dibahas:

```text
&&  ||  |  ?
```

Contoh:

```rupa
x = 1 + 2
x = 1 + 2 * 3
x = (1 + 2)
x = ((1 + 2) * (3 - 4))
x = 5 % 2
```

`|` pernah dibahas sebagai alternative value:

```rupa
value = first | second
```

## 12. Array

```rupa
[]
[1, 2, 3]
[1 + 2, 3 * 4]
[1, [2, 3], 4]
```

Bentuk yang masih berupa rencana dan belum boleh dianggap implementasi final:

```rupa
[...arr, 5]
[a, b] = [1, 2]
```

## 13. `print`

`print` adalah construct output:

```rupa
print("hello world")
print(name)
print("x =", x)
```

`print` menggunakan argument list yang sama dengan construct lain. Parameter dipisahkan dengan koma, sehingga expression dan pemanggilan bersarang tetap diproses melalui jalur delimiter/construct yang sama:

```rupa
print("x =", x)
print("result:", x + y, true)
print(add(1, 2), [3, 4])
```

Parenthesis setelah `print` adalah **argument list output**, bukan function call. Tidak ada scanner parameter khusus untuk `print`; lexer tetap memakai scanner construct dan delimiter yang sama agar nesting `()`, `[]`, dan expression tidak diproses dua kali.

## 14. Module-related syntax

Bentuk yang pernah dibahas:

```rupa
import sys from rupa
import sys, render, resources from rupa
import rupa.system as sys
```

`export` dan `extends` sudah menjadi bagian dari daftar keyword/desain, tetapi detail grammar dan semantiknya belum dianggap final.

## 15. Ringkasan makna `:`

Processor tidak boleh menganggap semua `:` sama:

```rupa
if x > 0: print(x)        // satu statement body
x: number = 1             // type annotation
name: "rupa"              // object property/value
```

Context aktif menentukan maknanya.

## 16. Status syntax

Dokumen ini membedakan dua hal:

- **Acuan aktif**: bentuk yang menjadi target lexer/processor saat ini.
- **Rencana**: bentuk yang pernah dibahas tetapi belum dianggap grammar final.

Saat menambah fitur baru, perbarui dokumen ini agar contributor berikutnya tidak harus menebak grammar dari source code.

## Decimal literal

Rupa does not decide `float` or `double` during lexing. A decimal literal such as `1.0` becomes `DECIMAL`; its runtime precision is resolved later from type annotation or inference:

```rupa
x: float = 1.0
y: double = 1.0
z = 1.0
```

`float` and `double` remain type annotations/runtime types, not literal token types.
