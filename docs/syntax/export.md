# Export

Export deklarasi dari file agar bisa di-import oleh file lain.

## Syntax

```rupa
// file modules/a.rp
create() {
    print("a.create called")
}

update() {
    print("a.update called")
}

export a
```

Export harus berada di akhir file. Nama export adalah nama yang digunakan saat import:

```rupa
import a from ./modules.a
```

## Contoh

### Export fungsi

```rupa
// file math.rp
add(a, b) {
    return a + b
}

sub(a, b) {
    return a - b
}

export math
```

### Import yang sudah di-export

```rupa
import add, sub from ./math
print(add(2, 3))  // 5
print(sub(5, 2))  // 3
```
