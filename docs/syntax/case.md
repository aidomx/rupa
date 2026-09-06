# Case

## Apa yang bisa ditulis?

```js
case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    default: print("unknown")
}
```

## Kapan digunakan?

Gunakan `case` untuk pattern matching. Setiap case memiliki pattern dan body. Wildcard `default` menangkap semua case yang tidak terpenuhi.

## Apa hasilnya?

Case mengevaluasi subject dan menjalankan body yang sesuai dengan pattern. Jika tidak ada yang cocok, wildcard dijalankan.

### Contoh execution

```js
status = 404

case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    default: print("unknown")
}
```

Output: `not found`

```js
status = 999

case status => {
    200: print("success")
    404: print("not found")
    500: print("server error")
    default: print("unknown")
}
```

Output: `unknown`
