# Module Grammar

Module grammar mengenali keyword:

```text
import
export
extends
```

Source test saat ini:

```rupa
import sys from rupa
import sys, render, resources from rupa
import rupa.system as sys
```

Parser membuat node module dengan jenis node berdasarkan keyword:

```text
NODE_IMPORT
NODE_EXPORT
NODE_EXTENDS
```

AST output test saat ini menampilkan `Module statement` dengan identifier value
yang dibentuk parser. Bentuk source modul sudah diuji, tetapi representasi
semantik lengkap seperti daftar nama, source module, dan alias masih perlu
diperluas bila AST module dikembangkan lebih jauh.

Dokumentasi ini hanya mengunci apa yang tampak dari parser dan AST saat ini,
bukan mendefinisikan semantik module yang belum direpresentasikan secara penuh.
