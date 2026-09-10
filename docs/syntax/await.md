# Await

`await` mengambil handle hasil `async` dan memastikan operasinya sudah selesai sebelum nilainya dipakai.

## Syntax

```rupa
await <expression>
```

`await` mengikat lebih erat dari operator binary/perbandingan, tapi lebih longgar dari member access (`.`):

- `await value != null` berarti `(await value) != null`
- `await users.data` berarti `(await users).data`

## Contoh

```rupa
users = async dbtest.getUser()
handle = await users
data = (await users).data
```

## Penjelasan

`await <expr>` mengevaluasi `<expr>`, lalu mengembalikan handle `{status, data, error}` yang sudah final (`SUCCESS` atau `ERROR` — lihat [async.md](async.md)).

`await` sendiri mengembalikan handle-nya, **bukan** langsung `data`-nya — pakai `.data` untuk mengambil hasilnya:

```rupa
users = async dbtest.getUser()
d = (await users).data
```

- `(await users)` → `{status: SUCCESS, data: [...], error: undefined}`
- `.data` → `[...]`
