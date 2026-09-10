# Async

## Apa yang bisa ditulis?

### Tanpa loader/timeout

```rupa
users = async db.getUser()
```

### Dengan loader dan timeout (angka langsung)

```rupa
loader() {
  if this.status == AWAIT:
    print("loading...")
  else if this.status == SUCCESS:
    print(this.data)
  else:
    print(this.error)
}

async db.getUser() -> {loader, 5000}
```

### Dengan loader dan timeout (referensi variabel)

```rupa
loader() {
  if this.status == SUCCESS:
    print(this.data)
}

timeout = 5000
async db.getUser() -> {loader, timeout}
```

## Kapan digunakan?

Gunakan `async` untuk menjalankan operasi yang hasilnya ingin ditangani lewat
pola status (`AWAIT` → `SUCCESS`/`ERROR`), bukan cuma nilai balik biasa.

`->` (then) di sini punya satu-satunya bentuk yang valid: `{loader, timeout}`.

- `loader` — nama function yang **sudah dideklarasikan terpisah** sebelumnya
  (lihat [function.md](function.md)). Bukan block statement, bukan ekspresi
  inline.
- `timeout` — angka literal langsung (`{loader, 5000}`) atau nama variabel
  yang sudah diisi angka (`{loader, timeout}`).

`->` boleh dihilangkan sama sekali kalau kamu hanya butuh menyimpan handle-nya
dulu dan membaca hasilnya belakangan lewat `await` (lihat
[await.md](await.md)).

## Hasilnya?

`async` mengembalikan handle dengan status `{status, data, error}`.

### Status

| Status    | Arti                             |
|-----------|-----------------------------------|
| `AWAIT`   | Request sedang berjalan           |
| `SUCCESS` | Request berhasil, `data` tersedia |
| `ERROR`   | Request gagal atau timeout        |

Kalau `loader` diberikan, ia dipanggil **dua kali**: sekali dengan
`this.status == AWAIT` (sebelum request berjalan), lalu sekali lagi dengan
`this.status == SUCCESS` atau `this.status == ERROR` (setelah request
selesai atau timeout terlampaui). Karena itu loader biasanya memeriksa
`this.status` untuk membedakan kedua pemanggilan tersebut.

### Contoh eksekusi

```rupa
import dbtest from rupa
import thread from rupa

loader() {
  if this.status == AWAIT:
    print("loading...\n")
  else if this.status == SUCCESS:
    print(this.data, "\n")
  else:
    print(this.error, "\n")
}

slowGetUsers() {
  thread.sleep(200)
  return dbtest.getUser()
}

// Request cepat, timeout longgar → SUCCESS
async dbtest.getUser() -> {loader, 5000}

// Request lambat, timeout ketat → ERROR
async slowGetUsers() -> {loader, 50}
```

Output:

```
loading...
[{id: 1, name: admin}, {id: 2, name: user}]
loading...
Request timed out
```

### Contoh dengan await

```rupa
import dbtest from rupa

users = async dbtest.getUser()
d = (await users).data
print(d, "\n")
```

`await` mengembalikan handle yang sudah diperbarui; `.data` mengambil hasil
darinya. Detail lengkap ada di [await.md](await.md).
