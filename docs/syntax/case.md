# Case

`case` digunakan untuk memilih aksi berdasarkan nilai yang sedang diperiksa.

Setiap nilai ditulis sebagai pattern diikuti `:` dan aksi yang akan dijalankan.

## Apa yang bisa ditulis?

### Memeriksa beberapa nilai

```rupa
case username => {
    "budi": print(true)
    "arman": print(true)
}
```

Jika nilai cocok dengan sebuah pattern, aksi pada pattern tersebut dijalankan.

### Menggunakan block

Jika satu cabang membutuhkan beberapa baris kode:

```rupa
case username => {
    "budi": {
        print("Welcome Budi")
        loadDashboard()
    }

    "arman": print("Welcome Arman")
}
```

### Menggunakan default

Gunakan `*` untuk nilai yang tidak cocok dengan pattern sebelumnya.

```rupa
case username => {
    "budi": print("Welcome Budi")
    "arman": print("Welcome Arman")
    *: print("Unknown user")
}
```

`*` ditulis sebagai cabang terakhir.

### Menggunakan `continue`

Jika `case` berada di dalam loop, `continue` dapat digunakan seperti biasa.

```rupa
for i < users.length {
    case users[i].username => {
        "budi": continue
        *: print(users[i].username)
    }
}
```

### Menggunakan `break`

`break` juga tetap mengikuti context luar.

```rupa
for i < users.length {
    case users[i].username => {
        "budi": break
        *: print(users[i].username)
    }
}
```

## Kapan digunakan?

Gunakan `case` ketika satu nilai perlu dibandingkan dengan beberapa kemungkinan.

```rupa
case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    *: print("unknown status")
}
```

`case` cocok digunakan ketika `if` dan `elseif` berulang untuk memeriksa nilai yang sama.

Daripada:

```rupa
if username == "budi":
    print("Budi")
elseif username == "arman":
    print("Arman")
else:
    print("Unknown")
```

dapat ditulis:

```rupa
case username => {
    "budi": print("Budi")
    "arman": print("Arman")
    *: print("Unknown")
}
```

## Apa hasilnya?

`case` memilih satu cabang berdasarkan pattern yang cocok.

```rupa
case username => {
    "budi": print("Budi")
    "arman": print("Arman")
    *: print("Unknown")
}
```

Jika `username` bernilai `"budi"`, hasilnya:

```text
Budi
```

Jika tidak ada pattern yang cocok, `*` digunakan sebagai default.

Setelah satu cabang selesai dijalankan, `case` juga selesai.

```text
nilai
  ↓
case
  ↓
cocok dengan pattern?
  ├── ya    → jalankan satu cabang → selesai
  └── tidak → periksa pattern berikutnya
                    ↓
                 * tersedia?
                    ├── ya    → jalankan default
                    └── tidak → selesai
```

Tidak ada fallthrough dan tidak perlu `break` untuk menghentikan perpindahan antar cabang.

`break` dan `continue` tetap digunakan untuk mengendalikan loop atau context luar, bukan mekanisme dasar `case`.
