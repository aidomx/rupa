# Memory Grammar

Grammar memory membentuk node call untuk alokasi dan pelepasan memori:
`new T(...)` dan `del(...)`, plus bentuk Contract lewat annotation dengan
call `new Contract()`.

Ciri khas grammar ini: **argumen pertama `new` adalah nama tipe, bukan
ekspresi** — parser menyimpannya mentah (identifier) dan runtime yang
membaca namanya. `new` dan `del` bukan keyword; keduanya callee biasa
sehingga memakai grammar call yang sama dengan fungsi lain.

## new T() — alokasi type-driven

Source:

```rupa
x = new Number()
arr = new Number(4)
q = new Number(arr, 8)
```

AST:

```text
Program:
  Assignment:
    Target:
      Identifier: x
    Value:
      Call:
        Callee:
          Identifier: new
        Arg 1:
          Identifier: Number
  Assignment:
    Target:
      Identifier: arr
    Value:
      Call:
        Callee:
          Identifier: new
        Arg 1:
          Identifier: Number
        Arg 2:
          Number: 4
  Assignment:
    Target:
      Identifier: q
    Value:
      Call:
        Callee:
          Identifier: new
        Arg 1:
          Identifier: Number
        Arg 2:
          Literal ID: arr
        Arg 3:
          Number: 8
```

Arg 1 selalu nama tipe (`Identifier` / `Literal ID`): `Number`,
`String`, `Boolean`, `Decimal`, `Ptr` untuk scalar (kapital), atau nama
struct untuk bentuk `new People()`. Arg 2..N adalah angka: `[n]` untuk
n elemen, `[src, n]` untuk realloc ke n elemen.

## del — free

Source:

```rupa
del(x)
del(a, b)
del([a, b])
```

AST:

```text
Program:
  Call:
    Callee:
      Identifier: del
    Arg 1:
      Literal ID: x
  Call:
    Callee:
      Identifier: del
    Arg 1:
      Literal ID: a
    Arg 2:
      Literal ID: b
  Call:
    Callee:
      Identifier: del
    Arg 1:
      ArrayLiteral:
        Literal ID: a
        Literal ID: b
```

Arity bebas — satu handle, variadic, atau array berisi handle
(`ArrayLiteral`). Statement-style: dipanggil tanpa assignment.

## new Contract() — anotasi sebagai spesifikasi alokasi

Source:

```rupa
p: number = new Contract()
list: number[] = new Contract(8)
```

AST:

```text
Program:
  Assignment:
    Target:
      Identifier: p
    Type:
      Identifier: number
    Value:
      Call:
        Callee:
          Identifier: new
        Arg 1:
          Identifier: Contract
  Assignment:
    Target:
      Identifier: list
    Type:
      ArrayType:
        Identifier: number
    Value:
      Call:
        Callee:
          Identifier: new
        Arg 1:
          Identifier: Contract
        Arg 2:
          Number: 8
```

Grammar-nya sama dengan `new T()` — yang membedakan ada di binding:
annotation membawa `Type`, dan runtime mengesankan alokasi ke tipe di
anotasi itu (arg `Contract` tidak menyebut tipe apa pun). Tanpa anotasi
→ error. Bentuk `ArrayType` pada anotasi menghasilkan raw block yang
diakses lewat subscript.

## Akses isi handle

Isi handle diakses dengan grammar subscript dan member yang sama dengan
tipe lain — tidak ada operator dereferensi khusus:

Source:

```rupa
p[0] = 42
v = p[0]
data.age = 30
```

AST:

```text
Program:
  MemberAssign:
    Target:
      Subscript:
        Base:
          Identifier: p
        Index:
          Number: 0
    Value:
      Number: 42
  Assignment:
    Target:
      Identifier: v
    Value:
      Subscript:
        Base:
          Identifier: p
        Index:
          Number: 0
  MemberAssign:
    Target:
      Member:
        Object:
          Identifier: data
        Member:
          Identifier: age
    Value:
      Number: 30
```

## Ringkasan grammar

| Bentuk | Node | Keterangan |
|--------|------|------------|
| `new T(...)` | Call (callee `new`) | Arg 1 = nama tipe, tidak dievaluasi |
| `del(...)` | Call (callee `del`) | Handle / variadic / `ArrayLiteral` |
| `p: T = new Contract()` | Assignment + Type | Tipe dari anotasi |
| `x[i]` pada handle | Subscript | Bounds check saat eksekusi |
| `obj.field` pada handle | Member / MemberAssign | Offset layout struct |
