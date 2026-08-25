# Rupa Documentation

Dokumentasi ini adalah pintu masuk utama untuk memahami dan berkontribusi pada Rupa.

Dokumentasi dipisahkan berdasarkan tujuan agar syntax, grammar, struktur proyek, dan cara kontribusi tidak tercampur dalam satu dokumen.

## Mulai dari sini

| Dokumen | Isi |
|---|---|
| [about.md](about.md) | Latar belakang dan sejarah Rupa |
| [mission.md](mission.md) | Tujuan utama proyek |
| [vision.md](vision.md) | Arah jangka panjang |
| [instruction.md](instruction.md) | Cara membangun, menjalankan, memahami proyek, dan berkontribusi |
| [structure.md](structure.md) | Struktur direktori dan tanggung jawab bagian proyek |
| [syntax/index.md](syntax/index.md) | Daftar syntax Rupa |
| [grammar/](grammar/) | Bentuk dan aturan syntax dari sisi sistem |

## Memahami Rupa

Urutan yang disarankan:

1. Baca [about.md](about.md) untuk mengenal latar belakang Rupa.
2. Baca [mission.md](mission.md) dan [vision.md](vision.md) untuk memahami arah proyek.
3. Baca [syntax/index.md](syntax/index.md) untuk melihat syntax yang tersedia.
4. Baca [grammar/](grammar/) jika bekerja pada lexer, processor, parser, AST, atau runtime.
5. Baca [structure.md](structure.md) untuk memahami susunan source.
6. Baca [instruction.md](instruction.md) sebelum membangun atau mengubah proyek.

## Syntax dan Grammar

Dokumentasi syntax dan grammar memiliki tujuan berbeda.

```text
docs/syntax/
→ cara pengguna menulis Rupa

docs/grammar/
→ bentuk dan aturan syntax dari sisi sistem
```

Dokumentasi syntax menjawab:

```text
Apa yang bisa ditulis?
Kapan digunakan?
Apa hasilnya?
```

Dokumentasi grammar menjelaskan bagaimana sebuah syntax dibentuk dan dipahami oleh sistem.

Jangan menganggap seluruh bentuk yang dibahas dalam grammar sudah selesai diimplementasikan hanya karena dokumentasinya sudah tersedia.

## Untuk Contributor

Sebelum mengubah kode:

1. Pahami tujuan perubahan.
2. Periksa dokumentasi syntax yang terkait.
3. Periksa grammar jika perubahan memengaruhi lexer, processor, parser, AST, atau runtime.
4. Periksa [structure.md](structure.md) sebelum menambah atau memindahkan source.
5. Build dan jalankan Rupa sebelum dan sesudah perubahan.
6. Uji perubahan dengan kasus sekecil mungkin yang benar-benar mewakili fitur yang diubah.

Prinsip umum:

```text
docs/syntax/
→ perilaku dan penggunaan dari sisi pengguna

docs/grammar/
→ kontrak bentuk dan perilaku dari sisi sistem

src/
lib/
include/
→ implementasi
```

Jika dokumentasi dan implementasi berbeda, jangan langsung mengubah salah satunya. Pastikan terlebih dahulu apakah perubahan yang diperlukan adalah pada desain syntax, grammar, atau implementasinya.

## Dokumentasi

```text
docs/
├── README.md
├── about.md
├── mission.md
├── vision.md
├── instruction.md
├── structure.md
├── syntax/
│   ├── index.md
│   └── ...
└── grammar/
    └── ...
```

`README.md` adalah pintu masuk dokumentasi. Setiap dokumen lain memiliki tanggung jawab yang lebih spesifik.
