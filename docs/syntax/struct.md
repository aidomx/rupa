# Struct

## Apa yang bisa ditulis?

```rupa
People {
    name: string
    age: number
}
```

## Kapan digunakan?

Gunakan struct untuk mendefinisikan tipe data dengan field yang telah ditentukan. Struct berfungsi sebagai blueprint untuk membuat object.

Untuk instance struct yang butuh operasi member (baca/tulis per field,
validasi layout saat diisi), gunakan `new Object` dengan struct sebagai
anotasinya — lihat bagian [Instance struct](#instance-struct-new-object) di bawah.

## Apa hasilnya?

Struct dibuat dengan nama diikuti block `{ }` yang berisi field declarations. Setiap field memiliki nama dan type annotation.

### Validasi type (analyzer)

Deklarasi struct berlaku sebagai kontrak type. Assignment / annotation yang
memakai nama struct divalidasi terhadap layout-nya:

```rupa
Point {
  x: number
  y: number
}

p: Point = { x: 10, y: 20 }   # OK
q: Point = { x: 10, y: "a" }  # TypeError: expected 'number', got 'string'
r: Nope = {}                  # TypeError: type 'Nope' tidak dikenal
```

Validasi bersifat rekursif (struct dalam struct, array of struct), dan field
struct dengan type yang tidak dikenal ditolak saat deklarasi. Tidak ada
forward reference antar struct — deklarasikan dulu sebelum dipakai.

### Instance struct (new Object)

`new Object(_, { ... })` membuat instance struct: field diisi langsung
dari object literal, tipe setiap field divalidasi terhadap layout, dan
instance membawa method member `has/get/set/update/delete`:

```rupa
People {
    name: string
    age: number
}

data: People = new Object(_, { name: "rudi", age: 20 })
print(data.name)
print(data.age)
```

Output:
```
rudi
20
```

Instance boleh kosong dulu, field diisi belakangan lewat `set` — argumen
pertama boleh nama field (tanpa tanda kutip) atau object literal untuk
merge:

```rupa
p: People = new Object()
p.set(name, "anggi")     # nama field langsung — tanpa kutip
p.set(age, 25)
p.set({ age: 26 })       # merge dari object literal
print(p.name)
print(p.age)
```

Output:
```
anggi
26
```

Akses member biasa dan validasi layout tetap berlaku:

```rupa
data.age = 21            # OK — tipe cocok
data.city = "x"          # TypeError: unknown field
```

Method member yang tersedia pada instance:

| Method | Bentuk | Hasil |
|--------|--------|-------|
| `has(k)` | string / nama field | true bila field ada |
| `get(k)` | string / nama field | nilai field |
| `set(k, v)` / `set({...})` | nama field atau object literal | tulis satu field / merge — divalidasi layout |
| `update({...})` | object literal | merge — divalidasi layout |
| `delete(k)` | string / nama field | hapus field |

### Ref — binding dua arah

`new Object(ref, init)` menghubungkan instance ke object lain: penulisan
lewat `set`/`update` terlihat di ref, dan member assign menembus untuk
field yang sudah ada di ref:

```rupa
a = new Object()
a.name = "awal"
b = new Object(a, {})    # b terhubung ke a
b.set(name, "ubah")
print(a.name)            # ubah — terlihat di ref
```

Output:
```
ubah
```

Tanpa argumen apa pun, `new Object()` membuat instance dinamis penuh
(tanpa kontrak struct) dengan method member yang sama.

### Contoh execution

```rupa
People {
    name: string
    age: number
}

person = { name: "Rupa", age: 20 }
print(person.name)
print(person.age)
```

Output:
```
Rupa
20
```

```rupa
Point {
    x: number
    y: number
}

p = { x: 10, y: 20 }
print(p.x)
print(p.y)
```

Output:
```
10
20
```
