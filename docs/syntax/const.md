# Const

## Apa yang bisa ditulis?

```rupa
const x: number = 1
const y = x + 2
```

`const` adalah keyword sebelum assignment. Dengan atau tanpa anotasi tipe
keduanya valid.

## Kapan digunakan?

Gunakan `const` untuk binding yang tidak boleh di-reassign setelah
inisialisasi — konstanta, nilai yang dihitung sekali, atau invariant.

## Apa hasilnya?

`const` adalah properti **slot/binding**, bukan properti nilai:

- Deklarasi `const x = v` menulis **dan mengunci** slot `x`.
- Assignment biasa (`x = ...`) ke slot terkunci ditolak saat eksekusi
  mencapai store tersebut — statement sebelum tetap berjalan normal.
- Deklarasi const di dalam loop / fungsi **re-init sah** di setiap
  iterasi atau call (slot lama di-reset, lalu dikunci ulang).

```rupa
const x: number = 1
print(x)   # 1
x = 10     # ConstError: cannot reassign const variable 'x'
```

## Pesan error

| Error       | Kondisi                                            |
| ----------- | -------------------------------------------------- |
| `ConstError` | Assignment biasa ke binding yang sudah const-locked |
| `TypeError`  | Anotasi tipe setelah `const` tidak cocok dengan nilai |
