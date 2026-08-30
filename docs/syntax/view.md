# View Module

## Apa yang bisa ditulis?

```rupa
import Render, Resources as R from rupa

main() {
    Render.view(R.layout.main)
}
```

## Kapan digunakan?

Gunakan view module untuk rendering UI. Module ini menyediakan fungsi untuk menampilkan layout dan komponen.

## Apa hasilnya?

View module memungkinkan rendering komponen UI. `Render.view()` menampilkan layout yang di指定.

### Contoh execution

```rupa
import Render from rupa

main() {
    Render.view("main")
}
```

Output: Rendering UI (tergantung implementasi)
