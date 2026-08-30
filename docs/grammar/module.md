# Module (Grammar)

Module terdiri dari dua bagian:

- [Import](import.md) — NODE_IMPORT dan NODE_MODULE_IMPORT
- [Export](export.md) — NODE_EXPORT

## Path Resolution

```
resolveModulePath("./modules.a") → "modules/a.rp"
resolveModulePath("modules.d")  → "modules/d.rp"
```
