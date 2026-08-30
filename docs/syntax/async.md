# Async

## Apa yang bisa ditulis?

```rupa
async request => {
    print("Request started")
} timeout 5000 {
    print("Request timed out")
}
```

## Kapan digunakan?

Gunakan `async` untuk menjalankan operasi asynchronous. Handler dijalankan saat operasi selesai, timeout handler dijalankan jika operasi melebihi batas waktu.

## Apahasilnya?

`async` menjalankan operasi secara asynchronous. Status tracking dibangun ke dalam bahasa.

### Contoh execution

```rupa
async request => {
    print("Async operation completed")
}
```

Output: `Async operation completed` (setelah operasi selesai)
