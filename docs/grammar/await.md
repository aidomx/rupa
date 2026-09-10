# Await Grammar

`await` adalah prefix expression yang mengambil handle hasil `async` dan
memastikan operasinya sudah selesai sebelum nilainya dipakai.

## Syntax

```
await <expression>
```

`await` mengikat lebih erat dari operator binary/perbandingan, tapi lebih
longgar dari member access (`.`):

- `await value != null` berarti `(await value) != null`
- `await users.data` berarti `(await users).data`

## Source

```rupa
users = async db.getUser()
handle = await users
```

AST:

```text
Assignment:
  Target:
    Identifier: handle
  Value:
    Await:
      Identifier: users
```

## Await structure

```text
Await
└── Expression  (ekspresi yang menghasilkan async handle)
```

## How it works

`await <expr>` mengevaluasi `<expr>`, lalu mengembalikan handle
`{status, data, error}` yang sudah final (`SUCCESS` atau `ERROR` — lihat
[async.md](async.md)). Karena `async` saat ini dievaluasi eager, handle-nya
sudah selesai pada saat `await` dipanggil.

`await` sendiri mengembalikan handle-nya, **bukan** langsung `data`-nya —
pakai `.data` untuk mengambil hasilnya:

```rupa
users = async dbtest.getUser()
d = (await users).data
```

`(await users)` → `{status: SUCCESS, data: [...], error: undefined}`
`.data` → `[...]`
