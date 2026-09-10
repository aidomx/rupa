# Print

## Apa yang bisa ditulis?

Mencetak teks, variabel, atau hasil ekspresi ke terminal.

```rupa
print('hello world');
```

## Dengan multiple arguments (dipisahkan koma):

```rupa
name = 'rupa';
x = 1;
y = 2;

print(name);
print(name + ' language');
print('x =', x);
print('result:', x + y, true);
```

## Function call dan array:

```rupa
add(x, y) {
    return x + y
}

print(add(1, 2), [3, 4])
```

## Interpolasi string

print() mendukung interpolasi di dalam string literal. Ada dua bentuk:

### `{variable}` — Variable interpolation

Single curly braces untuk menampilkan nilai variabel langsung:

```rupa
x = 10;
print('x = {x}'); // output: x = 10
print('{name}\n'); // output: nama + newline
```

### `{{expression}}` — Expression interpolation

Double curly braces untuk mengevaluasi ekspresi, termasuk function call dan member access:

```rupa
add(a, b) { return a + b }
print('{{add(1, 2)}}\n'); // output: 3 + newline

obj = { name: 'Rupa' };
print('{{obj.name}}'); // output: Rupa

print('{{x + y}}\n'); // output: hasil penjumlahan + newline
```

### Perbedaan `{ }` dan `{{ }}`

| Syntax     | Keterangan                                 | Contoh               |
| ---------- | ------------------------------------------ | -------------------- |
| `{x}`      | Variable lookup langsung                   | `"{x}"` → `10`       |
| `{{expr}}` | Ekspresi penuh (binary, call, member, dll) | `"{{x + y}}"` → `30` |

### AST Representation

String interpolation direpresentasikan sebagai node `StringInterp` di AST:

```text
Program:
  Print:
    StringInterp:
      String: "Hello "
      Call:
        Callee: Identifier: add
        Arg 1: Number: 1
        Arg 2: Number: 2
      String: "\n"
```

Node `StringInterp` berisi array parts yang ber чередaan antara `String` (literal) dan expression nodes.

---

## Kapan digunakan?

Gunakan print() untuk menampilkan output ke terminal.

· Multiple arguments dipisahkan koma.
· Interpolasi memudahkan penyisipan ekspresi di dalam teks.
· Gunakan `{x}` untuk variable sederhana, `{{x + y}}` untuk ekspresi kompleks.

---

## Apa hasilnya?

· print() tidak menambahkan newline otomatis. Gunakan \n untuk menambahkan newline secara eksplisit.
· Multiple arguments dicetak berurutan tanpa separator (spasi tidak ditambahkan otomatis).
· Tipe data ditampilkan sesuai representasinya (string, angka, boolean, array, objek, dll.).
· Ekspresi dalam `{ }` atau `{{ }}` dievaluasi dan hasilnya dikonversi ke string.

```rupa
print('hello'); // output: hello (tanpa newline)
print('hello\n'); // output: hello (dengan newline)
print('{x}'); // output: nilai x
print('{{x + y}}\n'); // output: hasil penjumlahan x+y + newline
```

---

## Contoh lengkap

```rupa
name = 'rupa';
x = 1;
y = 2;
obj = { key: 'value' };

print('hello world\n');
print(name + '\n');
print('x =', x, '\n');
print('{x}\n');
print('{{x + y}}\n');
print('obj.key = {obj.key}\n');
print('{{obj.key}}\n');
```

Output:

```
hello world
rupa
x = 1
1
3
obj.key = value
value
```

Arguments dicetak berurutan tanpa separator. Tipe data ditampilkan sesuai representasinya.
