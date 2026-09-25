# Tree Export — Rancangan Export/Import Berbasis Pohon Package

Status: rancangan terkunci dari temuan bug `/tmp/rpx_engine` + `/tmp/view.rp`.
Semua perilaku yang ditandai ✅ sudah diimplementasikan dan teruji.

## Model pohon

```
x/                      package root
├── index.rp            node cabang: export dari anak
└── y/                  sub-package
    ├── index.rp        node cabang
    └── test.rp         leaf: TIDAK wajib export
```

Aturan dasar:

1. **Leaf tidak perlu export.** `x/y/test.rp` cukup berisi kode. Ia hanya
   bisa dicapai MELALUI rantai index-nya — tidak pernah langsung dari luar
   package. ✅ (package boundary di loader.c)
2. **Index = node cabang.** `x/y/index.rp` men-ekspos isi leaf-nya
   (`export resources from ./resources`); `x/index.rp` men-ekspos
   sub-package-nya (`export view from ./view`). ✅
3. **Import dotted = navigasi pohon.** `import test from ./x.y.test`
   sesungguhnya berarti "akses sub-module `y.test` milik package `x`" —
   bukan file langsung. Rantai index dijalankan, hasilnya pohon object. ✅
4. **Tanpa double-export.** Jika `x/index.rp` me-re-export `x/y/index.rp`
   dan keduanya mengekspor sumber yang sama, module sumber tetap dieksekusi
   SEKALI per run (module cache per canonical path). Re-export hanya
   menyalin referensi value, bukan eksekusi ulang. ✅ (moduleCacheGet/Put)
5. **Circular tetap tertangkap.** Cache mencegah re-eksekusi; module-stack
   (canonical path) mencegah cycle antar file yang saling menunggu. ✅

## Policy `-> { name: private }`

- Berlaku di **setiap edge** pohon: saat leaf diekspos index-nya, dan saat
  index di-re-export oleh index induk.
- Multi-line didukung: `-> {` boleh dibuka di akhir baris statement export;
  parser memperluas range ke penutup brace seimbang. ✅
  (grammar_module.c — grammarLineEnd sadar-ARROW+brace)
- Enforcement: private member diganti `null` pada object hasil export dan
  dicatat di `_private`; `print()` tidak menampilkan keduanya sebagai
  member biasa. ✅

## Import dotted = redirect ke parent index ✅

Sesuai design/import_export.txt: `import resources from
./rpx_engine.view.resources` TIDAK diartikan akses langsung ke file leaf —
loader mengalihkannya ke `view/index.rp` dan entry import mengambil member
`resources` dari hasil export index cabang itu ("apakah ada sub module
yang di-export?"). Leaf dieksekusi sekali (pre-redirect cache) meski parent
me-load ulang untuk re-export.

Yang tetap ditolak:

```
import z from ./rpx_engine.z   // ImportError — z bukan cabang manapun:
                               // tidak ada z/ di package, melompati pohon
```

Bentuk lain yang sah: `import v from ./rpx_engine` lalu `v.view.resources`.

## Jaminan runtime

- **Module cache**: satu file = satu eksekusi per run program, berapa kali
  pun di-import (direct, re-export, atau berkali-kali dari consumer lain).
  Kunci = canonical path (realpath). ✅
- **ImportError spesifik**: module tanpa export (enforced), member tidak
  ada, module tidak ditemukan, stdlib package tidak ada — semua dilaporkan
  dengan lokasi statement import, exit code 1. ✅
- **Lokasi error bersih**: error dari dalam module tidak mencemari lokasi
  global statement caller (snapshot/restore). ✅

## Binding hasil `import` — hidden di whole-env export ✅

Binding yang berasal dari statement `import` **bukan milik module** — ia
hanya konsumen. Maka saat module di-publish sebagai whole-env (atau lewat
rantai tree export), binding import **selalu hidden**:

- `import fs, os from rupa` di dalam leaf → `fs`/`os` tidak muncul di
  object yang dilihat consumer (`resources.fs` → `undefined`).
- Yang terekspos hanya yang **didefinisikan** module tersebut
  (fungsi, const, class, dsb).
- Trace saat development cukup `print(fs)` di dalam module sendiri —
  tidak perlu mekanisme opt-in.

Detail penting:
- Hanya **module object** (bare/alias/namespace `import db from rupa`)
  yang ditandai hidden (`RuntimeBinding.isImport`).
- **Member leaf** (`import has, get from ./drivers` di driver.rp stdlib)
  dan hasil **wildcard flatten** (`d.*`) TETAP publik — itu pola
  komposisi package yang memang harus mengalir ke consumer berikutnya.

Catatan historis: sempat dirancang opt-in `-> { import: public }` dan
self-entry `export . -> { ... }`; keduanya dicabut — default hidden
tanpa pengecualian lebih sederhana dan cukup.

## Export wildcard folder: `.` / `*` / `.*` — desain terkunci, belum diimplementasikan

Bentuk ringkas untuk index yang hanya me-re-export isi foldernya — tanpa
menulis `export x from ./x` satu per satu. Ditulis di dalam `index.rp`
(bisa polos atau di dalam namespace):

```rupa
namespace view { export . }    // satu tingkat
namespace view { export .* }   // rekursif
```

Semantik dilihat dari folder tempat `index.rp` berada (mis. `view/`):

| Bentuk | Arti |
|--------|------|
| `.` | Semua member **setingkat** `view/index.rp`: file langsung di `view/` + folder anak langsung — KECUALI `view/index.rp` itu sendiri. Setara dengan menulis export-nya satu per satu di index. |
| `*` | Semuanya yang ada di `view/`, kecuali `view/index.rp`. |
| `.*` | Gabungan keduanya sebagai **pembatas kedalaman**: rekursif — tidak peduli kedalaman, selama ada di folder `view/` (index.rp tetap dikecualikan). |

Kombinasi `.` + `*` penting sebagai pembatas: `.` sendiri berhenti di satu
tingkat, tambahan `*` barulah membuka kedalaman. Tanpa pembatas itu, `.`
dan `.*` mudah tertukar.

Jaminan yang ikut berlaku (tidak berubah dari model pohon):

- Nama member mengikuti nama file/folder (penamaan rantai index yang ada).
- Leaf tanpa export tetap terjangkau hanya lewat index foldernya — wildcard
  TIDAK membuka akses langsung dari luar package (package boundary tetap).
- Binding `import` di tiap file tetap hidden; policy `-> { x: private }`
  per-edge tetap bisa ditambahkan menyertai wildcard.
- Module cache: tiap file tetap dieksekusi SEKALI per run meski tersentuh
  lewat beberapa jalur wildcard.

Catatan implementasi: sisa eksperimen self-entry `export .`
(grammar_module_export.c → `modEntry(".")`) telah DIHAPUS dari parser
bersamaan dengan pembersihan bentuk warisan — `.` kini bebas dipakai untuk
arti barunya: "isi folder setingkat index". `*`/`.*` menyusul sebagai
variasi kedalaman.

## Bentuk kanonik terimplementasi, bentuk warisan dihapus ✅

Empat bentuk kanonik design/import_export.txt hidup; bentuk warisan
**dihapus dari bahasa** — parser menolak dan runtime melaporkan ImportError
pengarahan ke bentuk kanonik:

- `import * as x from ./X` / `import *` (flatten) — entry star bernama kosong.
- `import x, y from ./X` — selective nama polos.
- `export * from ./X` (dengan/tanpa policy) — flatten di dispatch + loader.
- `export x, y from ./X` — selective.
- `export name from ./path` member-first: binding bernama sama di source
  menang atas whole module (menjaga API fungsi seperti `db.open`), fallback
  whole module untuk merangkai pohon (leaf/sub-package).
- Dihapus: bare import/export, member chain `x.y`, wildcard member `x.*`,
  entry alias `x as y`, source alias `from X as m` — parser menolak
  (GRAMMAR_NO_MATCH → fallback no-op), runtime tanpa pesan migrasi.
  `resolveImplicitExportEntry` dan jalur `!mod->source` di
  `computeExportBindings` ikut dibuang (dead code).
- stdlib & tests termigrasi; arsip embedded (`modules/rupa_modules.tar.gz`)
  wajib di-rebuild (`make bin/rupa`) setelah mengubah stdlib — runtime
  memakai arsip, bukan fixture stdlib/.

## Pola komposisi stdlib — jangan diubah

Wrapper `stdlib/fs/fs.rp` (`import fsbase from rupa` + satu fungsi
passthrough per operasi) BUKAN teknis penantian restrukturisasi — pola ini
justru yang diinginkan:

- `fsbase` ber-flag `isImport` → hidden dari consumer; yang terekspos hanya
  11 fungsi wrapper = API publik `fs` yang bersih.
- Lapisan native tidak pernah bocor ke permukaan module.

Pola yang sama dipakai stdlib lain (`database/driver.rp`: `import has, get
from ./drivers` — member leaf tetap publik). Konsisten dengan aturan
hidden-import di atas.
