# Built-in Functions

Fungsi bawaan yang tersedia tanpa import.

## type(value)

Mengembalikan tipe data sebagai string.

```rupa
print(type(5))        // number
print(type(3.14))     // decimal
print(type("hello"))  // string
print(type(true))     // boolean
print(type(null))     // null
print(type([1,2]))    // array
print(type({a: 1}))   // object
print(type(fn() {}))  // function
```

| Tipe | Return |
|------|--------|
| `5` | `"number"` |
| `3.14` | `"decimal"` |
| `"hello"` | `"string"` |
| `true` | `"boolean"` |
| `null` | `"null"` |
| `[1,2]` | `"array"` |
| `{a: 1}` | `"object"` |
| `fn() {}` | `"function"` |

## len(value)

Mengembalikan panjang string, array, atau jumlah key object.

```rupa
print(len("hello"))     // 5
print(len([1, 2, 3]))   // 3
print(len({a: 1, b: 2})) // 2
```

## isNull(value)

Mengembalikan `true` jika value null.

```rupa
print(isNull(null))    // true
print(isNull(0))       // false
print(isNull(""))      // false
```

## toNumber(value)

Konversi value ke number.

```rupa
print(toNumber("42"))    // 42
print(toNumber("3.14"))  // 3
print(toNumber(true))    // 1
print(toNumber(null))    // 0
```

## toString(value)

Konversi value ke string.

```rupa
print(toString(42))     // "42"
print(toString(true))   // "true"
print(toString(null))   // "null"
```

## sizeof(TypeName)

Ukuran representasi tipe dalam byte. Argumennya **nama tipe**, bukan value
— menerima scalar, struct terdaftar, dan bentuk array `T[]`.

```rupa
Point { x: number, y: number }

print(sizeof(number))   // 8
print(sizeof(decimal))  // 8
print(sizeof(boolean))  // 1
print(sizeof(string))   // 8  (char*)
print(sizeof(ptr))      // 8  (void*)
print(sizeof(Point))    // 16 (jumlah field scalar)
print(sizeof(number[])) // 16 (representasi array: items + length)
```

| Tipe | Ukuran |
|------|--------|
| `number` | 8 |
| `decimal` | 8 |
| `boolean` | 1 |
| `string` | 8 |
| `ptr` | 8 |
| `Struct` | jumlah ukuran field (rekursif untuk struct bertingkat) |
| `T[]` | representasi array (16) |

> `number` = 8 byte (64-bit, sejak 2026-09-18); sebelumnya 4.

## new Contract() — anotasi sebagai spesifikasi alokasi

Tipe disebut **sekali**, di anotasi — tanpa `new Object()/new
String()/new Array()`. Satu form untuk semua penampung:

```rupa
p: number      = new Contract()    // 1 elemen number, zeroed
buf: number    = new Contract(4)   // 4 elemen — sizeof(buf) == 16
list: number[] = new Contract(8)   // blok 8 elemen (raw block)
data: People   = new Contract()    // sizeof(People) zeroed

buf[2] = 7
list[1] = 99
p = "str"           // TypeError — kontrak type permanen
```

| Aturan | Catatan |
|--------|---------|
| Wajib anotasi | `p = new Contract()` polos → TypeError (kontraknya anotasi itu) |
| `T[]` = raw block | akses `list[i]` via indexing — satu model dengan scalar |
| Struct member access | `data.age = 30` / `print(data.age)` — offset dari layout struct, tipe field dicek |
| Registry v3 | nama tipe elemen tersimpan di registry → view check = lookup |

**Member access struct** — field scalar (`number/decimal/boolean`) baca-
tulis native via layout offset; field tak dikenal ditolak:

```rupa
data: People = new Contract()
data.age = 30
data.score = 3.14
data.active = true
print(data.age)          // 30
data.ghost = 1          // TypeError: unknown field
```

**Nested struct & array of struct** — field struct bertingkat diakses
sebagai view handle ke posisi field-nya (kepemilikan tetap di blok
owner), jadi rantai member bekerja penuh. Elemen array-of-struct juga
view — akses field per elemen:

```rupa
Inner { x: number }
Outer {
    inner: Inner
    y: number
}

o: Outer = new Contract()
o.inner.x = 42
print(o.inner.x)         // 42

Point {
    x: number
    y: number
}
arr: Point = new Contract(4)
arr[0].x = 5
print(arr[1].x)          // 0 — elemen lain tak tersentuh
```

Layout struct mengikuti C: field diselaskan pada alignment-nya
(`sizeof(Mixed)` dengan `boolean` + `number` = 16), total size kelipatan
alignment terbesar. `del` pada view handle ditolak — hapus blok ownernya.

Indexing elemen tetap tersedia: `data[0]`, dan handle nested
(`p2: Point = new Contract()`; `p2.x + p2.y`) bekerja penuh.

**String slot first-class** — field/variable `string` pada handle
Contract benar-benar menyimpan teks: assignment menulis ke slot
(write-through), pembacaan membaca slot (read-through):

```rupa
name: string = new Contract()   // slot string, kosong
name = "rudi"                   // tulis ke slot (gcstrdup internal)
name = dupl("rudi", 3)          // tulis ptr hasil dupl ke slot
print(name)                     // read-through → "rudi"
type(name)                      // "string" — handle ter-transparansi penuh
```

RHS **string atau ptr hasil dupl** → tulis ke slot; RHS **ptr lain** →
rebind handle biasa. `compare(name, "rudi")` membaca slot.

## new/del — alokasi type-driven (GC-tracked)

Lapisan alokasi utama: bahasa MEMANFAATKAN pengetahuan tipe — ukuran
otomatis `sizeof(T)`, `n` = jumlah **elemen** (byte tidak pernah muncul
di API), hasil zeroed. Global — tanpa import.

```rupa
x = new Number()        // 1 elemen (zeroed)
x[0] = 42               // scalar = buffer 1 elemen; index 0
print(x[0])             // 42

arr = new Number(4)     // 4 elemen
arr[2] = 7

big = new Number(100 * 100)        // n bisa ekspresi apa pun
bigger = new Number(big, 200 * 100) // realloc — handle lama dangling

del(x)                  // free — statement-style, tanpa assignment
del(a, b)               // variadic
del([a, b])             // dari array
```

| Bentuk | Semantik |
|--------|----------|
| `new T()` | 1 elemen, zeroed |
| `new T(n)` | n elemen, zeroed |
| `new T(src, n)` | realloc ke n elemen — handle lama dangling |
| `del(x, ...)` / `del([x, y])` | free variadic / dari array |

**Kapitalisasi**: `new Number()`, bukan `new number()` — `new number()`
bentrok dengan penanda tipe `x: number`. Bentuk kapital berlaku untuk
scalar (`Number/String/Boolean/Decimal/Ptr`); struct pakai nama aslinya
(`new People()`).

**Akses isi** — scalar = buffer 1 elemen; `x[0]` baca/tulis dengan
bounds check dari registry (ukuran blok tersimpan per alokasi):

```rupa
p = new Number(3)
p[3] = 1          // RangeError: out of bounds
q = new Number(p, 8)
q[7] = p[0]       // realloc selalu pindah — handle p dangling setelahnya
```

**Kontrak type permanen** — `x = new Number()` mencatat declared type
`number` di binding, sehingga `x = "str"` ditolak TypeError sepanjang
umur variable.

**Guard** — `del` pada pointer bukan milik GC / double-free →
`MemoryError` + eksekusi berhenti. `sizeof(p)` pada handle
mengembalikan ukuran blok terdaftar (12 untuk `new Number(3)`).

## pin family — DIHAPUS (legacy)

`pin`, `elpin`, `repin`, `repins`, dan `unpin` sudah dihapus dari
runtime — memanggilnya → `TypeError`.

Penggantinya di halaman ini: [`new/del`](#new-del-alokasi-type-driven-gc-tracked)
dan [`new Contract()`](#new-contract-anotasi-sebagai-spesifikasi-alokasi)
— satu pintu alokasi type-driven dengan view check otomatis dari
registry (tanpa `sizeof` di setiap site).

## Operasi blok & string — di atas handle Contract

Operasi blok bekerja pada handle Contract yang sudah ada (posisi tulis
wajib ptr milik GC; posisi baca menerima ptr GC **atau** string biasa).
`dupl` mengalokasikan blok string baru yang terdaftar di GC.

```rupa
p = new Contract(8)             // 8 elemen number, zeroed
cset(p, 0, 64)                  // memset — clear buffer
q = ccpy(p, "rupa", 5)          // memcpy — string sebagai sumber baca
q == p                          // true — return dest

cmove(p, src, 16)               // memmove — aman overlap
compare(p, other, 16)           // memcmp — < 0 / 0 / > 0

s = dupl("hello")               // strdup ke blok string GC-tracked
t = dupl("hello world", 5)      // strndup — maksimal 5 char
compare(s, "hello")             // 0 jika sama — dua arity
```

| Fungsi | Padanan C | Return |
|--------|-----------|--------|
| `ccpy(dest, src, n)` | memcpy | `dest` (chainable) |
| `cmove(dest, src, n)` | memmove (aman overlap) | `dest` (chainable) |
| `cset(ptr, value, n)` | memset | `ptr` (chainable) |
| `dupl(str)` / `dupl(str, n)` | strdup / strndup | ptr (GC-tracked) |
| `compare(a, b)` / `compare(a, b, n)` | memcmp | number (< 0 / 0 / > 0) |

**Guard** — menulis ke pointer yang bukan milik GC → `MemoryError`
(dan menghentikan eksekusi). `compare` boleh membandingkan dua string
biasa karena read-only.

Catatan: `sizeof` menerima nama tipe — dan sejak registry v2 juga
*handle* (`sizeof(p)` → ukuran blok terdaftar).
