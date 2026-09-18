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

Field kompleks (`array/struct`) pada buffer scalar menghasilkan
snapshot object read-only — buffer menyimpan byte mentah, bukan char*.
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

| Bentuk | Semantik | Padanan pin era |
|--------|----------|-----------------|
| `new T()` | 1 elemen, zeroed | elpin(1, sizeof(T)) |
| `new T(n)` | n elemen, zeroed | elpin(n, sizeof(T)) |
| `new T(src, n)` | realloc ke n elemen | repin(src, n·sizeof(T)) |
| `del(x, ...)` / `del([x, y])` | free variadic / dari array | gcfree |

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

## pin family — sistem memori (GC-tracked)

Keluarga alokasi memori yang terdaftar di registry GC. Global — tanpa
import.

```rupa
p: ptr = pin(sizeof(number))      // malloc + register (tak diinisialisasi)
a: number[] = elpin(4, sizeof(number))  // calloc + register (zeroed)
p = repin(p, 64)                  // realloc — registry mengikuti
p = repins(p, 8, sizeof(number))  // reallocarray — NULL jika overflow
```

| Fungsi | Padanan C | Catatan |
|--------|-----------|---------|
| `pin(size)` | malloc | mengembalikan null jika gagal |
| `elpin(count, size)` | calloc | memori di-nol-kan |
| `repin(ptr, size)` | realloc | pointer lama tidak valid setelah call — selalu reassign |
| `repins(ptr, count, size)` | reallocarray | tolak jika `count * size` overflow |

**View type check** — anotasi menentukan cara handle dibaca (per TYPE,
bukan per byte):

```rupa
x: number = pin(sizeof(number))   // OK
y: number = pin(sizeof(string))   // TypeError: number vs string
z: number = pin(64)               // TypeError: generic ptr (tanpa sizeof)
w: ptr    = pin(64)               // OK — ptr = handle generik
q: number = repin(x, 32)          // OK — inherit provenance
```

Pointer yang tidak dimiliki GC ditolak oleh repin/repins (`MemoryError`).
Cek kegagalan alokasi dengan `p == null`.

## Operasi blok & alokasi string (GC-tracked)

Operasi blok bekerja pada memori yang sudah ada (posisi tulis wajib
ptr milik GC; posisi baca menerima ptr GC **atau** string biasa).
Alokasi string menghasilkan ptr baru yang terdaftar di GC.

```rupa
p = elpin(8, sizeof(number))
setpin(p, 0, 32)                // memset — clear buffer
q = copypin(p, "rupa", 5)       // memcpy — string sebagai sumber baca
q == p                          // true — return dest

movepin(a, b, 8)                // memmove — aman overlap
pincmp(a, b, 8)                 // memcmp — < 0 / 0 / > 0

d = dupin("hello")              // strdup — ptr baru terdaftar GC
m = maxdupin("hello world", 5)  // strndup — maksimal 5 char, NUL-terminated

s = dupl("hello")               // dupin baru — nama pendek, satu nama dua arity
t = dupl("hello world", 5)      // strndup baru — maksimal 5 char
compare(s, "hello")             // pincmp baru — 0 jika sama, dua arity
```

| Fungsi | Padanan C | Return |
|--------|-----------|--------|
| `copypin(dest, src, n)` | memcpy | `dest` (chainable) |
| `movepin(dest, src, n)` | memmove (aman overlap) | `dest` (chainable) |
| `setpin(ptr, value, n)` | memset | `ptr` (chainable) |
| `pincmp(a, b, n)` | memcmp | number (< 0 / 0 / > 0) |
| `dupin(str)` | strdup | ptr (GC-tracked) |
| `maxdupin(str, n)` | strndup | ptr — selalu NUL-terminated (n + 1 byte) |
| `unpin(ptr)` | free | statement-style: `unpin(p)`, tanpa assignment |
| `dupl(str)` / `dupl(str, n)` | strdup / strndup | ptr (GC-tracked) |
| `compare(a, b)` / `compare(a, b, n)` | memcmp | number (< 0 / 0 / > 0) |

`dupl`/`compare` adalah nama baru untuk `dupin`/`maxdupin`/`pincmp` —
satu nama, dispatch by arity. Nama lama masih berfungsi.

**Guard** — menulis ke pointer yang bukan milik GC → `MemoryError`
(dan menghentikan eksekusi). `pincmp` boleh membandingkan dua string
biasa karena read-only.

`unpin(p)` membebaskan blok lebih awal (setara `free`) — registry
menghapus alamat sehingga double-free tertangkap sebagai `MemoryError`.
Kebutuhan utama tetap `gcclean` di akhir program; `unpin` ada untuk
berjaga-jaga saat blok besar perlu dirilis sebelum selesai dipakai.
Handle lama menjadi dangling setelah `unpin` — jangan dipakai lagi.

Catatan: `sizeof` menerima nama tipe — dan sejak registry v2 juga
*handle* (`sizeof(p)` → ukuran blok terdaftar). `setpin` versi lama di
design pointer (`setpin(s, len, size)`) sudah digantikan bentuk memset
`(dst, byte, n)` ini.

> pin/elpin/repin/repins kini **deprecated by design** — gunakan
> `new/del` (lapisan type-driven di atas). Block ops & dup family tetap
> sebagai lapisan byte rendah di atas handle.
