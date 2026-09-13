# Import

Import kode dari file lain ke dalam scope saat ini. Import statements harus berada di bagian atas file.

## 1. Specific functions

Import fungsi tertentu dari module. Fungsi langsung tersedia di top-level.

```rupa
import create, update from ./modules.a

create()
update()
```

## 2. Wildcard dengan namespace

Import semua fungsi dari beberapa module ke dalam namespace.

```rupa
import b.*, c.*, d.* from ./modules as m

m.login()
m.services()
m.auth()
```

Wildcard flatten semua fungsi ke dalam namespace — `b.login` menjadi `m.login`.

## 3. Wildcard dengan alias

Import semua fungsi dari module, bind sebagai object dengan nama alias.

```rupa
import a.* as form from ./modules

form.create()
form.update()
```

## 4. Mixed import

Wildcard dan specific function dalam satu statement.

```rupa
import a.* as form, b.login, b.register from ./modules

form.create()   // alias object
login()         // top-level
register()      // top-level
```

## 5. Full module import

Import seluruh module sebagai nama. Parser mencoba load file langsung, lalu fallback ke sub-module.

```rupa
import d from ./modules

d.auth()
d.users()
```

## 6. Stdlib import

Import dari standard library `rupa`.

```rupa
import os from rupa
print(os.info().user)
```

## 7. Stdlib specific function

Import fungsi tertentu dari stdlib module.

```rupa
import info from rupa.os
print(info())

import server, stop from rupa.http
s = server(8080)
stop(s)
```

## 8. Stdlib wildcard

Import semua fungsi dari stdlib module dengan namespace.

```rupa
import http.*, thread.* from rupa as r

r.http.server(8080)
r.thread.sleep(100)
```


## 9. Import namespace dari stdlib

Module stdlib dapat menyediakan namespace sebagai entry point public:

```rupa
// stdlib/database/index.rp
namespace db {
  export driver.*
}
```

Import namespace tersebut menggunakan nama namespace, bukan nama directory:

```rupa
import db from rupa

db.use(...)
db.hasDriver(...)
```

`export driver.*` melakukan flatten sehingga API `driver` langsung menjadi
property dari `db`. Jika terdapat namespace di dalam namespace, aksesnya tetap
bertingkat:

```text
ns.ns    → namespace di dalam namespace
ns.props → property/function langsung
```

## Ringkasan

| Syntax | Hasil |
|--------|-------|
| `import X from ./path.a` | `X()` top-level |
| `import X.*, Y.* from ./path as ns` | `ns.func()` |
| `import X.* as alias from ./path` | `alias.func()` |
| `import X.a, X.b from ./path` | `a()`, `b()` top-level |
| `import X from ./path` | `X.func()` |
| `import X from rupa` | Namespace/module `X` dari stdlib |
| `import X from rupa.Y` | Fungsi dari module |
| `import X, Y from rupa.Y` | Beberapa fungsi |
| `import X.*, Y.* from rupa as ns` | Wildcard + alias |
