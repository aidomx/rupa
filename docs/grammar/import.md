# Import (Grammar)

## NODE_IMPORT

Old-style import — `import X from path`

### Specific functions

```rupa
import create, update from ./modules.a
```

```
Module statement:
  Import:
    Value: Array
      Literal ID: create
      Literal ID: update
    Name: Literal ID: modules.a
```

Interpreter:
1. Load `modules/a.rp`
2. Extract `create` dan `update`
3. Bind di top-level

### Full module import

```rupa
import d from ./modules
```

```
Module statement:
  Import:
    Value: Literal ID: d
    Name: Literal ID: modules
```

Interpreter flow:
1. Coba `loadModuleFile("modules")` → `modules.rp`
2. Jika gagal, coba `loadModuleFile("modules.d")` → `modules/d.rp`
3. Bind seluruh module sebagai `d`

### Stdlib import

```rupa
import os from rupa
```

```
Module statement:
  Import:
    Value: Literal ID: os
    Name: Literal ID: rupa
```

Interpreter:
1. Cari di stdlib modules → `os`
2. Bind di top-level

## NODE_MODULE_IMPORT

Flat import — `import entries from path as alias`

### Wildcard dengan namespace

```rupa
import b.*, c.*, d.* from ./modules as m
```

```
Module statement:
  Module Import:
    Base Path: Literal ID: modules
    Alias: Literal ID: m
    Entry 0:
      Path: Literal ID: b
      Wildcard: true
      Alias: -
    Entry 1:
      Path: Literal ID: c
      Wildcard: true
      Alias: -
    Entry 2:
      Path: Literal ID: d
      Wildcard: true
      Alias: -
```

Interpreter — flatten ke namespace:
```
m.login = b.login
m.register = b.register
m.services = c.services
m.auth = d.auth
m.users = d.users
```

### Wildcard dengan per-entry alias

```rupa
import a.* as form from ./modules
```

```
Module statement:
  Module Import:
    Base Path: Literal ID: modules
    Alias: - (no namespace)
    Entry 0:
      Path: Literal ID: a
      Wildcard: true
      Alias: Literal ID: form
```

Interpreter — `form` = module object:
```
form = { create: <fn>, update: <fn>, delete: <fn> }
```

### Mixed import

```rupa
import a.* as form, b.login, b.register from ./modules
```

```
Module statement:
  Module Import:
    Base Path: Literal ID: modules
    Alias: - (no namespace)
    Entry 0:
      Path: Literal ID: a
      Wildcard: true
      Alias: Literal ID: form
    Entry 1:
      Path: Literal ID: b.login
      Wildcard: false
      Alias: -
    Entry 2:
      Path: Literal ID: b.register
      Wildcard: false
      Alias: -
```

Interpreter:
- Wildcard + alias → `form` = module object
- Specific function → extract dari module, bind top-level

## Path Resolution

```
resolveModulePath("./modules.a") → "modules/a.rp"
resolveModulePath("modules.d")  → "modules/d.rp"
```

`./` prefix di-strip oleh `resolveModulePath`.

## NODE_MOD (design baru — belum aktif)

> Status: **design selesai, factory siap** (`createModImport`/`createModExport` di
> `src/compiler/parser/node/factory_nodes.c`). Migrasi grammar/interpreter/formatter
> belum dilakukan — node lama (`NODE_IMPORT`, `NODE_MODULE_IMPORT`, `NODE_EXPORT`,
> `NODE_EXPORT_DECL`) masih dipakai runtime.

1 container untuk 2 job: import dan export. Semua alokasi via GC
(`gccalloc`/`gcstrdup`), tanpa free manual.

### Struct

```
AstModEntry (enum ModEntryKind type, name, key, value, childrens)
AstMod      (enum ModType type, entries[], source, sourceAlias, policies[])
```

| Field | Isi | Contoh |
|-------|-----|--------|
| `type` (entry) | `MOD_ID` / `MOD_MEMBER` / `MOD_WILD` | `x` / `x.y` / `x.*` |
| `name` | nama sumber entry | `a`, `login` |
| `key` | binding lokal `x as y` → `"y"`; NULL = pakai name | `import a.* as form` → `key:"form"` |
| `value` | payload policy `{ a: private }` → `"private"`; selain itu NULL | policy |
| `childrens` | sub-path `a.create` → rantai entry `{name:"create"}` | `MOD_MEMBER` |
| `type` (AstMod) | `ImportDecl` / `ExportDecl` | — |
| `source` | path module; **NULL = export lokal** (`export x`) | `"../modules"`, `"rupa.os"` |
| `sourceAlias` | `from X as m` → `"m"` | namespace |
| `policies` | `export c from Y -> { a: private }` | array entry |

### Tabel Pemetaan Sintaks (oracle untuk round-trip test)

| Sintaks | entries | source | sourceAlias | policies |
|---|---|---|---|---|
| `import create, update from ../modules.a` | `{ID:create}`, `{ID:update}` | `../modules.a` | - | - |
| `import b.*, c.*, d.* from ../modules as m` | `{WILD:b}`, `{WILD:c}`, `{WILD:d}` | `../modules` | `m` | - |
| `import a.* as form from ../modules` | `{WILD:a, key:form}` | `../modules` | - | - |
| `import a.* as form, b.login from ../modules` | + `{MEMBER:b → [login]}` | `../modules` | - | - |
| `import d from ../modules` | `{ID:d}` | `../modules` | - | - |
| `import info from rupa.os` | `{ID:info}` | `rupa.os` | - | - |
| `export x` | `{ID:x}` | **NULL** | - | - |
| `export a, b from ../c` | `{ID:a}`, `{ID:b}` | `../c` | - | - |
| `export c from ../c -> { a: private }` | `{ID:c}` | `../c` | - | `{name:a, value:private}` |

### Rencana Migrasi

1. Grammar emit `NODE_MOD` (node lama tetap ada sebagai deprecated)
2. Formatter & printer tambah `case NODE_MOD` — round-trip tabel pemetaan
3. Interpreter binding `NODE_MOD` paralel dengan jalur lama
4. Hapus 4 node lama + struct lama dalam satu commit
