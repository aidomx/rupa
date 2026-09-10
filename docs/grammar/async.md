# Async Grammar

Grammar async membentuk node async dari request dan konfigurasi
`{loader, timeout}`.

## Syntax

```
async <request>
async <request> -> {loader, timeout}
```

`->` di sini adalah grammar then biasa (lihat [then.md](then.md)), bukan
block handler. Isi `{}` di sisi kanannya **selalu** pasangan `loader` dan
`timeout`, dua slot dipisah koma — tidak ada bentuk lain (bukan block
statement, bukan ekspresi tunggal).

- `loader` — referensi identifier ke function yang sudah dideklarasikan
  terpisah (lihat [function.md](function.md)); dipanggil dengan
  `this = {status, data, error}` setiap kali status berubah.
- `timeout` — angka literal langsung (`{loader, 5000}`) atau referensi
  identifier ke variabel yang sudah didefinisikan (`{loader, timeout}` dengan
  `timeout = 5000` di baris lain).

`->` boleh tidak dituliskan sama sekali; request tetap dievaluasi, hanya
tanpa loader/timeout (lihat [await.md](await.md) untuk cara mengambil
hasilnya lewat `await`).

## Async tanpa handler

Source:

```rupa
users = async db.getUser()
```

AST:

```text
Assignment:
  Target:
    Identifier: users
  Value:
    Async:
      Request:
        Call:
          Callee:
            Member:
              Object:
                Identifier: db
              Member:
                Identifier: getUser
```

## Async dengan loader dan timeout

Source:

```rupa
loader() {
  if this.status == AWAIT:
    print("loading...\n")
  else if this.status == SUCCESS:
    print(this.data, "\n")
  else:
    print(this.error, "\n")
}

async dbtest.getUser() -> {loader, 5000}
```

AST:

```text
Async:
  Request:
    Call:
      Callee:
        Member:
          Object:
            Identifier: dbtest
          Member:
            Identifier: getUser
  Loader:
    Identifier: loader
  TimeoutId:
    Number: 5000
```

Timeout juga boleh berupa referensi variabel alih-alih angka literal
langsung:

```rupa
timeout = 5000
async dbtest.getUser() -> {loader, timeout}
```

```text
Async:
  Request: ...
  Loader:
    Identifier: loader
  TimeoutId:
    Identifier: timeout
```

## Async structure

```text
Async
├── Request    (expression yang dievaluasi)
├── Loader     (identifier reference ke loader function, -1 jika tidak ada)
└── TimeoutId  (Number atau Identifier reference, -1 jika tidak ada)
```

## How it works

1. Request dievaluasi secara eager.
2. Jika `loader` ada, dipanggil dengan `this = {status: AWAIT, data: null, error: null}`
   sebelum request dievaluasi.
3. Setelah request selesai (atau timeout terlampaui), `loader` dipanggil lagi
   dengan `this = {status: SUCCESS|ERROR, data, error}`.
4. Hasil akhir async expression adalah handle `{status, data, error}` yang
   sama — bisa ditangkap lewat assignment dan dibaca lagi lewat `await`
   (lihat [await.md](await.md)).

Karena `loader` bisa dipanggil lebih dari sekali (fase AWAIT lalu fase
SUCCESS/ERROR), loader idealnya memeriksa `this.status` untuk membedakan
setiap pemanggilan — seperti contoh di atas.
