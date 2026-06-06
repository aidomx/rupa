# Vision

Visi Rupa sederhana namun kuat:
**menjadi bahasa yang mampu bertahan dalam segala konteks,
kuat dalam fondasi, dan fleksibel untuk setiap kebutuhan.**

Rupa tidak hanya sekadar bahasa pemrograman,
tetapi fondasi untuk membangun ide, sistem, dan masa depan.

## Future Direction

Salah satu arah pengembangan Rupa di masa depan adalah mendukung
**penulisan sintaks dalam bahasa Ibu**.
Dengan menggunakan direktif `#lang`, programmer dapat menulis kode
dengan gaya penulisan sesuai bahasa lokal, tanpa mengorbankan kompatibilitas global.

Contoh:

```rupa
#lang: id_ID
jika x == 10 {
  tulis(x)
}
```

```rupa
#lang: pt_BR
se x == 10 {
  escreva(x)
}
```

Tanpa `#lang`, Rupa akan menggunakan **bahasa Inggris** sebagai standar global:

```rupa
if x == 10 {
  print(x)
}
```

> **Satu bahasa, banyak cara bicara.**
