# Member Grammar

Member expression mengakses property atau method dari sebuah value.

## Bentuk umum

```text
member → expression `.` identifier
call   → member `(` arguments? `)`
```

Contoh:

```rupa
name.length
name.upper()
```

Keduanya dibentuk dari member access. Perbedaannya adalah `name.upper()` kemudian diproses sebagai `NODE_CALL`.

## AST

Source:

```rupa
name.upper()
```

Struktur konseptual AST:

```text
Call
└── Callee
    └── Member
        ├── Object
        │   └── Identifier: name
        └── Member
            └── Identifier: upper
```

Member evaluator mengevaluasi object terlebih dahulu, kemudian mencari nama member pada value tersebut.

## String property

Untuk `VALUE_STRING`, property `length` menghasilkan jumlah karakter:

```rupa
name.length
```

Secara runtime:

```text
VALUE_STRING → Member("length") → VALUE_NUMBER
```

`length` bukan native function dan tidak memerlukan `()`.

## Bound string methods

Member evaluator menyediakan native method yang terikat pada string receiver:

```rupa
name.upper()
name.lower()
name.trim()
name.contains("x")
name.startsWith("R")
name.endsWith("a")
name.replace("R", "r")
```

Secara internal, receiver disimpan pada native function sementara:

```text
receiver: name
method:   upper
```

Saat call dieksekusi, runtime menyusun argument menjadi:

```text
[name, explicit arguments...]
```

Dengan demikian:

```rupa
name.upper()
```

setara secara semantik dengan:

```rupa
string.upper(name)
```

Tetapi parser tetap menghasilkan member lalu call, bukan mengubah source menjadi pemanggilan module secara tekstual.

## Method dan property

| Source | Hasil grammar | Hasil runtime |
|---|---|---|
| `name.length` | `NODE_MEMBER` | number |
| `name.upper()` | `NODE_CALL` dengan callee `NODE_MEMBER` | string |
| `name.contains("x")` | `NODE_CALL` dengan callee `NODE_MEMBER` | boolean |

## Chaining

Karena method string menghasilkan string baru, member expression dapat dirangkai:

```rupa
name.trim().upper()
```

Strukturnya bersarang dari kiri ke kanan:

```text
Call
└── Member: upper
    └── Call
        └── Member: trim
            └── Identifier: name
```

## Jenis value lain

Array juga memiliki property bawaan:

```rupa
items.length
```

Aturan member bersifat type-directed. Jika property atau method tidak tersedia untuk tipe value, evaluator menghasilkan error type mismatch atau `null` sesuai aturan runtime yang berlaku.
