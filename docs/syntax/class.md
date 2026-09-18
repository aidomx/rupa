# Class

## Apa yang bisa ditulis?

Class dideklarasikan **tanpa keyword** — penandanya `: Type` yang merujuk
struct yang sudah ada:

```rupa
MonsterType {}

Monster: MonsterType {
    construct() {
        this.set(null)
    }
}
```

## Kapan digunakan?

Gunakan class ketika struct butuh **perilaku** (method), bukan hanya data.
Struct = data, class = presentation di atas struct. Deklarasi `main extends
Monster {}` juga membentuk class — `extends` otomatis menandai class tanpa
perlu anotasi tambahan.

## Apa hasilnya?

`Name: Type {}` menghasilkan deklarasi class: nama class, type yang dirujuk,
dan body berisi method. Di AST dikenali sebagai node `Class` (terpisah dari
`Struct`), dan formatter mempertahankan anotasi `: Type` saat round-trip
(`rupa fmt`).

### Catatan status

Fitur dalam pengembangan. Yang sudah aktif:

- Parsing `Name: Type {}` → node Class (interpreter + IR).
- Registrasi type + layout field sejajar struct.
- Method di body dikenali sebagai function declaration.

Yang belum aktif: `this`/closure di runtime, `super` dispatch, `@created`,
`@input`. Nama constructor ditetapkan `construct()`.
