# Main Entry Point

## Apa yang bisa ditulis?

```rupa
main() {
    print("Hello from Rupa!")
}
```

Dengan arguments:

```rupa
main(args: const) {
    print(args.length)
}
```

## Kapan digunakan?

`main` adalah entry point program. Runtime akan mencari fungsi global bernama `main` dan menjalankannya secara otomatis.

## Apa hasilnya?

`main` dieksekusi saat program dimulai. Parameter `args` bersifat opsional dan menyediakan akses ke command line arguments.

### Contoh execution

```rupa
main() {
    print("Hello from Rupa!")
}
```

Output: `Hello from Rupa!`

```rupa
main(args: const) {
    print("Jumlah argumen:", args.length)
    for arg in args {
        print(arg)
    }
}
```

Output (dengan argumen):
```
Jumlah argumen: 2
--help
--version
```
