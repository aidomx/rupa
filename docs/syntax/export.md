# Export

Export deklarasi dari file agar bisa di-import oleh file lain.

## Syntax

### 1. Selective Export — `export a, b from ./path`

Mengekspor item tertentu dari module lain:

```rupa
// file modules/math.rp
add(a, b) { return a + b }
sub(a, b) { return a - b }
mul(a, b) { return a * b }

// file index.rp — hanya export add dan sub
export add, sub from ./math
```

```rupa
import add, sub from ./index
print(add(2, 3))  // 5
print(sub(5, 2))  // 3
// mul tidak bisa di-import — tidak di-export
```

### 2. Module Namespace Export — `export name from ./path`

Mengekspor seluruh module sebagai namespace dengan nama custom:

```rupa
// file modules/math.rp
add(a, b) { return a + b }
sub(a, b) { return a - b }

// file index.rp
export math_utils from ./math
```

```rupa
import math_utils from ./index
print(math_utils.add(2, 3))  // 5
print(math_utils.sub(5, 2))  // 3
```

> Nama namespace (`math_utils`) tidak harus sama dengan nama file (`math.rp`).

### 3. Policy Export — `export name from ./path -> { item: private }`

Mengekspor module dengan akses kontrol item tertentu:

```rupa
// file modules/auth.rp
login(user, pass) { ... }
register(user, pass) { ... }
hashPassword(pass) { ... }   // internal — tidak boleh diakses luar

// file index.rp
export auth from ./auth -> { hashPassword: private }
```

```rupa
import auth from ./index
auth.login("admin", "123")      // ✅ works
auth.register("user", "456")    // ✅ works
auth.hashPassword("secret")     // ❌ PrivateError: 'hashPassword' is private and cannot be called from outside
```


### 3. Namespace Declaration — `namespace name { ... }`

Namespace mengelompokkan export ke dalam satu object namespace. Namespace dapat
berisi namespace lain atau export module.

```rupa
namespace db {
  export driver.*
}
```

Jika `driver` memiliki `hasDriver`, `getDriver`, dan `use`, wildcard `.*`
melakukan **flatten** ke namespace `db`:

```rupa
db.hasDriver(...)
db.getDriver(...)
db.use(...)
```

`db.driver.use(...)` tidak digunakan karena `driver.*` sudah mengambil seluruh
isi `driver` ke dalam `db`.

Tanpa `.*`, namespace tetap bertingkat:

```rupa
namespace db {
  namespace driver {
    export use
  }
}
```

Akses mengikuti pola:

```text
ns.ns       → namespace di dalam namespace
ns.props    → property/function langsung pada namespace
```

Namespace juga dapat digunakan sebagai public entry point module. Contohnya
`stdlib/database/index.rp` dapat mendefinisikan `namespace db`, lalu digunakan
dengan:

```rupa
import db from rupa

db.use(...)
```

> Nama directory/package tidak harus sama dengan nama namespace yang diekspor.

## Aturan

| Syntax | Keterangan |
|--------|------------|
| `export a, b from ./c` | Hanya `a` dan `b` yang di-export dari `c` |
| `export c from ./c` | Seluruh binding di `c` di-export sebagai namespace `c` |
| `namespace c { ... }` | Membentuk namespace `c` dari export di dalam body |
| `export c.*` | Flatten seluruh isi namespace/module `c` ke namespace saat ini |
| `export c from ./c -> { x: private }` | Namespace `c` di-export, tapi `x` tidak bisa diakses |
| `export math` | Legacy — export semua binding (backward compatible) |

### Behavior Detail

- **Source module tidak perlu punya `export` statement** sendiri. Export dideklarasikan di file yang melakukan import (`index.rp`).
- **Private items** tidak bisa dipanggil dari luar. Jika dipanggil, muncul `PrivateError` yang spesifik.
- **`_private` metadata** disimpan secara internal di module object dan tidak ditampilkan saat `print()`.

## Error Cases

- **Module tanpa export**: Jika file tidak memiliki `export`, nothing is exported (return `null`).
- **Import private item**: Jika mencoba memanggil item yang di-mark `private`, muncul `PrivateError: '<name>' is private and cannot be called from outside`.
- **Import dari module kosong**: Jika module tidak export apapun, import akan error.

## Path Resolution

| Path | Resolves to |
|------|-------------|
| `./modules.a` | `./modules/a.rp` |
| `./modules` | `./modules/*.rp` |
| `rupa` | stdlib module |
