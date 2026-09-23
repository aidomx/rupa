# Internal — pin family (DIHAPUS)

> **Dokumen internal (src/docs, tidak di-publish).** Status: **DIHAPUS
> dari runtime.** Fungsi `pin`, `elpin`, `repin`, `repins`, dan `unpin`
> tidak lagi tersedia — memanggilnya menghasilkan `TypeError`.
>
> Penggantinya di docs pengguna: `new/del` dan `new Contract()`
> (`docs/syntax/builtins.md`). Dasar keputusan: `design/new_memory.txt`
> (Replace + deprecated) dan `design/str_memory.txt` ("new Contract()
> sudah sangat cocok"). View type check handle kini berbasis registry
> v3 (`gcregtype`) — provenance `sizeof` jadi legacy bersama pin
> family-nya. Block ops di-rename: `cset`/`cmove`/`ccpy`.

---

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
bukan per byte) via provenance `sizeof` di argumennya:

```rupa
x: number = pin(sizeof(number))   // OK
y: number = pin(sizeof(string))   // TypeError: number vs string
z: number = pin(64)               // TypeError: generic ptr (tanpa sizeof)
w: ptr    = pin(64)               // OK — ptr = handle generik
q: number = repin(x, 32)          // OK — inherit provenance
```

Pointer yang tidak dimiliki GC ditolak oleh repin/repins (`MemoryError`).
Cek kegagalan alokasi dengan `p == null`.

## unpin — free eksplisit (REVISI: tetap diimplementasi, kini ikut dihapus)

`unpin(p)` membebaskan blok lebih awal (setara `free`/`gcfree`) —
statement-style, tanpa assignment. Registry menghapus alamat sehingga
double-free tertangkap sebagai `MemoryError`. Kebutuhan utama tetap
`gcclean` di akhir program; `unpin` ada untuk berjaga-jaga saat blok
besar perlu dirilis sebelum selesai dipakai. Handle lama menjadi
dangling setelah `unpin` — jangan dipakai lagi.

**Pengganti kini: `del(p)`** — guard GC yang sama (double-free
terdeteksi), satu pintu dengan alokasi `new`.

## Padanan ke era new/del

| Pin family | Pengganti | Catatan |
|------------|-----------|---------|
| `pin(sizeof(T))` | `new T()` | 1 elemen, zeroed, size otomatis |
| `elpin(n, sizeof(T))` | `new T(n)` | n elemen, zeroed |
| `repin(p, n·sizeof(T))` | `new T(p, n)` | realloc ke n elemen; handle lama dangling |
| `repins(p, n, sizeof(T))` | `new T(p, n)` | overflow-cek internal via `sizeof(T)` |
| `unpin(p)` | `del(p)` | free + guard GC identik |

Block ops (`cset/cmove/ccpy`, dulu `setpin/movepin/copypin`) dan dup
family (`dupl/compare`) **bukan bagian** yang dihapus — mereka lapisan
byte rendah di atas handle dan tetap didokumentasikan di
`docs/syntax/builtins.md`.

`dupin`/`maxdupin` (prekursor `dupl`) masih berfungsi di runtime,
namun **tidak di-expose di docs** — gunakan `dupl(str)` / `dupl(str, n)`
yang dispatch by arity (lihat `design/str_memory.txt` S3).
