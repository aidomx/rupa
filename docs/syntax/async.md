# Async

`async` digunakan untuk menjalankan pekerjaan tanpa menghentikan alur program.

Hasil dari pekerjaan `async` dapat digunakan langsung dengan `await` ketika hasil tersebut dibutuhkan.

## Apa yang bisa ditulis?

### Menjalankan pekerjaan secara asynchronous

```rupa
users = async db.getUser()
```

`db.getUser()` mulai berjalan secara asynchronous.
Program tidak harus langsung menunggu hasilnya.

```rupa
users = async db.getUser()

print("Welcome to dashboard")
```

`Welcome to dashboard` dapat dijalankan tanpa menunggu `db.getUser()` selesai.

### Menunggu hasil dengan await

Gunakan `await` pada bagian yang benar-benar membutuhkan hasil dari pekerjaan asynchronous.

```rupa
users = async db.getUser()

data = await users.data
```

Contoh di dalam kondisi:

```rupa
if await users.data != null:
    print(true)
```

Contoh di dalam perulangan:

```rupa
for i < await users.data.length:
    print(users.data[i].id)
```

`await` dapat digunakan pada expression yang membutuhkan hasil asynchronous.

### Mengirim hasil async ke function

Hasil async dapat digunakan atau dikirim ke function lain.

```rupa
tableUsers(users) {
    for i < await users.data.length:
        print(users.data[i].id)
}

users = async db.getUser()

tableUsers(users)
```

Function yang menerima hasil async tidak perlu dideklarasikan secara khusus.

### Menggunakan handler

Jika hasil request cukup ditangani langsung, variabel tidak perlu dibuat.

```rupa
async db.getUser() => {
    if this.status == AWAIT:
        print("loading...")
    elseif this.status == SUCCESS:
        print(this.data)
    else:
        print(this.error.message)
} -> 200
```

Handler singkat juga dapat ditulis tanpa block:

```rupa
async db.getUser() => print(this.data.length) -> 200
```

Di dalam handler, `this` mengacu pada hasil pekerjaan asynchronous yang sedang ditangani.

### Menggunakan timeout

Batas waktu dapat ditambahkan menggunakan `->`.

```rupa
users = async db.getUser() -> 200
```

Atau bersama handler:

```rupa
async db.getUser() => handler -> 200
```

## Kapan digunakan?

Gunakan `async` ketika sebuah pekerjaan membutuhkan waktu dan program tidak perlu berhenti untuk menunggunya.

Contohnya:

```rupa
users = async db.getUser()
```

Program dapat melanjutkan pekerjaan lain:

```rupa
users = async db.getUser()

print("Welcome")
print("Loading dashboard")
```

Gunakan `await` hanya ketika hasil benar-benar dibutuhkan.

```rupa
users = async db.getUser()

print("Welcome")

for i < await users.data.length:
    print(users.data[i].username)
```

Pada contoh tersebut, program tidak perlu menunggu saat request dimulai.
`for` baru menunggu ketika membutuhkan `users.data`.

Gunakan handler ketika seluruh lifecycle pekerjaan ingin ditangani langsung:

```rupa
async api.login(username, password) => {
    if this.status == AWAIT:
        print("loading...")
    elseif this.status == SUCCESS:
        print("login successfully")
    else:
        print(this.error.message)
} -> 200
```

## Apa hasilnya?

`async` menghasilkan hasil asynchronous yang dapat digunakan di tempat lain.

```rupa
users = async db.getUser()
```

Hasil tersebut memiliki informasi tentang pekerjaan:

```text
users
├── status
├── data
└── error
```

### Status

`status` menunjukkan keadaan pekerjaan.

- `AWAIT`
- `SUCCESS`
- `FAILED`

Contoh:

```rupa
users = async db.getUser()

if users.status == AWAIT:
    print("loading...")
```

### Data

`data` berisi hasil dari pekerjaan.

```rupa
users = async db.getUser()

data = await users.data
```

Isi `data` bergantung pada nilai yang dikembalikan oleh pekerjaan.

Misalnya jika `db.getUser()` mengembalikan daftar pengguna:

```rupa
for i < await users.data.length:
    print(users.data[i].username)
```

Jika mengembalikan object dengan count:

```rupa
count = await users.data.count
```

Struktur data tidak ditentukan oleh `async`.

### Error

Jika pekerjaan gagal, informasi kegagalan tersedia melalui `error`.

```rupa
users = async db.getUser()

result = await users

if result.status == FAILED:
    print(result.error.message)
```

Jika hanya membutuhkan data:

```rupa
data = await users.data
```

`data` dapat bernilai `null` ketika tidak ada hasil data yang tersedia.

Jika membutuhkan seluruh informasi hasil, tunggu handle-nya:

```rupa
result = await users

print(result.status)
print(result.data)
print(result.error)
```

## Contoh

Contoh menampilkan data pengguna:

```rupa
import db from database

// menampilkan semua data pengguna
tableUsers() {
    print("No | Username | Action")

    users = async db.getUser()

    for i < await users.data.length:
        print(i + 1)
        print(users.data[i].username)
        print(users.data[i].id)
}

tableUsers()
```

Alurnya:

```text
async db.getUser()
        ↓
pekerjaan berjalan

for membutuhkan users.data
        ↓
await users.data
        ↓
hasil belum siap?
├── ya    → tunggu
└── tidak → jalankan for
```

## Ringkasan

**`async`**
→ mulai pekerjaan tanpa langsung menghentikan alur program

**`await`**
→ tunggu hasil pada titik yang benar-benar membutuhkannya

**`async request => handler`**
→ tangani lifecycle pekerjaan secara langsung

**`->`**
→ menentukan batas waktu pekerjaan

`async` tidak membuat function menjadi jenis function khusus.
`await` juga tidak mengharuskan function atau scope menjadi asynchronous.

```rupa
tableUsers() {
    users = async db.getUser()

    for i < await users.data.length:
        print(users.data[i].id)
}
```

Asynchronous berada pada hasil pekerjaan, sedangkan `await` menentukan tempat hasil tersebut benar-benar diperlukan.
