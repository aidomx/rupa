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

## Ringkasan

| Syntax | Hasil |
|--------|-------|
| `import X from ./path.a` | `X()` top-level |
| `import X.*, Y.* from ./path as ns` | `ns.func()` |
| `import X.* as alias from ./path` | `alias.func()` |
| `import X.a, X.b from ./path` | `a()`, `b()` top-level |
| `import X from ./path` | `X.func()` |
| `import X from rupa.Y` | stdlib |
