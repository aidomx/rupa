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
