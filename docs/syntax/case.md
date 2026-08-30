# Case

## Apa yang bisa ditulis?

```rupa
case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    *: print("unknown")
}
```

## Kapan digunakan?

Gunakan `case` untuk pattern matching. Setiap case memiliki pattern dan body. Wildcard `*` menangkap semua case yang tidak terpenuhi.

## Apa hasilnya?

Case mengevaluasi subject dan menjalankan body yang sesuai dengan pattern. Jika tidak ada yang cocok, wildcard dijalankan.

### Contoh execution

```rupa
status = 404

case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    *: print("unknown")
}
```

Output: `not found`

```rupa
status = 999

case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    *: print("unknown")
}
```

Output: `unknown`
