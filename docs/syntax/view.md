# View Module

View adalah unit UI yang dapat didefinisikan di module terpisah, diekspor, lalu dipasang ke layout melalui `Render.view`.

## Mendefinisikan view

Contoh `src/header.rp`:

```rupa
export view header() {
}
```

`view` menandai bahwa `header` adalah unit yang dapat dirender, sedangkan `export` membuatnya tersedia bagi module lain.

View dapat menerima data read-only:

```rupa
export view header(data: const) {
    return {
        title: data.title
    }
}
```

## Mengimpor dan menggunakan view

Contoh `src/main.rp`:

```rupa
import header from src.header
import Render, Resources as R from rupa

main() {
    Render.view(R.layout.main, R.id.header, header)
}
```

Maknanya:

1. `src.header` menyediakan view `header`.
2. `R.layout.main` menunjuk layout yang dirender.
3. `R.id.header` menunjuk anchor di dalam layout.
4. `header` dipasang ke anchor tersebut.

## Resource layout

Layout dapat menyediakan anchor untuk view:

```html
<!-- resources/layout/main.html -->
<header id="@header" />
```

`@header` adalah resource anchor. Compiler dapat mengeksposnya sebagai:

```rupa
R.id.header
```

Dengan begitu, kode Rupa tidak perlu memakai selector string secara langsung.

## View dengan data

View dapat menerima context atau props dari pemanggil:

```rupa
export view header(data: const) {
    return {
        title: data.title
    }
}
```

```rupa
import header from src.header
import Render, Resources as R from rupa

main() {
    Render.view(
        R.layout.main,
        R.id.header,
        header,
        {
            title: "Rupa"
        }
    )
}
```

Data dari runtime sebaiknya diperlakukan sebagai read-only melalui `const`.

## Resource dan view

Resource dapat berisi konten langsung:

```html
<header id="@header">
    <h1>Loading...</h1>
</header>
```

Atau hanya menyediakan anchor kosong:

```html
<header id="@header" />
```

Dengan demikian, layout dapat memiliki fallback statis atau sepenuhnya diisi oleh view Rupa.

## Event handler

Event handler dapat diekspor dari module yang sama:

```rupa
export view header(data: const) {
    return {
        title: data.title
    }
}

export onclick headerClick(event: const) {
    print("Header clicked")
}
```

Kemudian digunakan oleh entry point:

```rupa
import header, headerClick from src.header
import Render, Resources as R from rupa

main() {
    Render.view(R.layout.main, R.id.header, header)
    Render.onclick(R.id.header, headerClick)
}
```

Hubungan view dan event juga dapat direpresentasikan dengan annotation resource:

```rupa
@view(R.id.header, header)
header(data: const) {
}

@onclick(R.id.header, headerClick)
headerClick(event: const) {
}
```

Fungsi view atau handler dapat diletakkan di mana saja. Compiler mengumpulkan deklarasi dan metadata annotation terlebih dahulu, kemudian runtime mendaftarkannya saat aplikasi dimulai.

## Tujuan desain

Model ini memisahkan tanggung jawab:

| Bagian | Tanggung jawab |
|---|---|
| HTML resource | Struktur layout dan anchor |
| `R.layout.*` | Referensi layout |
| `R.id.*` | Referensi element/anchor |
| `view` | Unit UI yang dapat digunakan kembali |
| `Render.view` | Memasang view ke layout |
| `onclick` | Behavior saat event terjadi |
| `main` | Entry point dan komposisi aplikasi |

View module tidak harus terikat pada satu layout tertentu. Module dapat hanya mengekspor view, sementara `main` menentukan layout dan anchor tempat view tersebut digunakan.
