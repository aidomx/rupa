# Rupa Documentation

Dokumentasi ini adalah pintu masuk utama untuk memahami dan berkontribusi pada Rupa. Sebelum mengubah kode, baca dokumen yang sesuai agar perubahan tetap mengikuti arah bahasa dan struktur proyek.

## Mulai dari sini

| Dokumen | Isi |
|---|---|
| [about.md](about.md) | Latar belakang dan sejarah Rupa |
| [mission.md](mission.md) | Tujuan utama proyek |
| [vision.md](vision.md) | Arah jangka panjang |
| [syntax.md](syntax.md) | Syntax dan grammar yang sedang menjadi acuan |
| [instruction.md](instruction.md) | Cara build, menjalankan, struktur kode, dan kontribusi |

## Untuk contributor

Urutan yang disarankan:

1. Baca `mission.md` dan `vision.md` untuk memahami arah proyek.
2. Baca `syntax.md` sebelum mengubah lexer, token, processor, parser, atau runtime.
3. Baca `instruction.md` sebelum mengubah struktur proyek atau menambahkan source baru.
4. Build dan jalankan Rupa sebelum dan sesudah perubahan.
5. Uji perubahan dengan kasus sekecil mungkin yang benar-benar mewakili fitur yang diubah.

> `docs/syntax.md` adalah acuan syntax. `src/`, `lib/`, dan `include/` adalah implementasi. Jika keduanya berbeda, jangan langsung mengubah salah satunya tanpa memastikan apakah syntax atau implementasinya yang memang perlu diperbaiki.
