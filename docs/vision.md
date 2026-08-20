# Vision

Visi Rupa adalah menjadi bahasa yang mampu bertahan dan berkembang dalam berbagai konteks tanpa kehilangan fondasi dan identitasnya.

## Arah jangka panjang

Rupa diarahkan untuk memiliki fondasi yang cukup dekat dengan sistem agar dapat berkembang ke kebutuhan low-level, sekaligus cukup nyaman untuk construct tingkat tinggi.

Salah satu arah masa depan adalah dukungan penulisan syntax dalam bahasa lokal melalui direktif seperti `#lang`, tanpa mengorbankan representasi atau kompatibilitas global.

Contoh konsep:

```rupa
#lang: id_ID
jika x == 10 {
    tulis(x)
}
```

Tanpa direktif bahasa, syntax standar menggunakan bentuk global yang ditetapkan Rupa:

```rupa
if x == 10 {
    print(x)
}
```

Contoh `#lang` adalah arah desain masa depan, bukan klaim bahwa fitur tersebut sudah diimplementasikan.

> Satu bahasa, banyak cara berbicara.
