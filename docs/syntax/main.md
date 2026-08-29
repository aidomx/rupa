# Main Entry Point

`main` adalah fungsi entry point program Rupa. Runtime akan mencari fungsi global bernama `main` dan menjalankannya secara otomatis ketika program dimulai.

Rupa tidak membutuhkan keyword khusus atau return type untuk menandai entry point.

## Tanpa argumen

Untuk aplikasi yang tidak membutuhkan input saat startup, gunakan `main()`:

```rupa
import Render, Resources as R from rupa

main() {
    Render.view(R.layout.main)
}
```

Bentuk ini dapat digunakan untuk aplikasi web, desktop, atau target lain yang tidak membutuhkan argumen command line.

## Dengan argumen

Parameter `args` bersifat opsional:

```rupa
main(args: const) {
    print(args.length)
}
```

`args` menyatukan informasi yang pada C biasanya dipisahkan menjadi `argc` dan `argv`:

| Rupa | C |
|---|---|
| `args.length` | `argc` |
| `args[index]` | `argv[index]` |

Contoh:

```rupa
main(args: const) {
    print("Jumlah argumen:", args.length)

    for arg in args {
        print(arg)
    }
}
```

## `const` pada `args`

`const` berarti argument runtime hanya dapat dibaca. Program tidak boleh mengganti binding `args` atau mengubah elemen inputnya:

```rupa
main(args: const) {
    print(args[0])

    # Tidak valid:
    # args = otherArgs
    # args[0] = "changed"
}
```

Jika program membutuhkan data yang dapat diubah, buat salinan lokal terlebih dahulu.

## Lintas target

`main` tidak khusus untuk web. Contoh penggunaannya dapat berbeda sesuai target program:

```rupa
# Web
main() {
    Render.view(R.layout.main)
}
```

```rupa
# CLI
main(args: const) {
    if args.length > 0 {
        print(args[0])
    }
}
```

```rupa
# Worker
main(args: const) {
    Worker.run(args)
}
```

## Aturan dasar

Bentuk yang didukung untuk entry point adalah:

```rupa
main()
main(args: const)
```

Parameter `args` dirancang sebagai optional agar program sederhana tetap ringkas, sementara program CLI, worker, desktop, atau target lain dapat menerima input startup ketika sistem runtime sudah mendukungnya.

`main` adalah konvensi entry point, bukan keyword baru. Nama tersebut tetap dapat diproses khusus oleh runtime tanpa menambah syntax deklarasi tambahan.
