# Modules

Dokumentasi modul bawaan Rupa. Semua modul diakses melalui import:

```rupa
import math from rupa
import os from rupa
import { join, base } from rupa.sys
```

Modul yang bisa ditulis dengan bahasa Rupa ditulis di `stdlib/`
(mis. `math`, `sys`); yang butuh akses sistem ditulis sebagai native
module di `src/stdlib/` (mis. `os`, `io`, `thread`).

## Daftar modul

| Modul | Import | Fungsi utama |
|-------|--------|--------------|
| [math](math.md) | `import math from rupa` | abs, min, max, floor, ceil, round, sqrt, pow, random |
| [os](os.md) | `import os from rupa` | exec, getcwd, getenv, chdir, mkdir, listdir, info |
| [io](io.md) | `import io from rupa` | input, toNumber |
| [json](json.md) | `import json from rupa` | stringify, parse |
| [string](string.md) | `import string from rupa` | split, join, trim, replace, dsb. |
| [thread](thread.md) | `import thread from rupa` | create, join, sleep |
| [http](http.md) | `import http from rupa` | request, server (listen, route) |
| [datetime](datetime.md) | `import datetime from rupa` | now, format, timestamp |
| [regex](regex.md) | `import regex from rupa` | match, test, replace, groups |
| [crypto](crypto.md) | `import crypto from rupa` | hash (sha256, md5), hmac, random |
| [net](net.md) | `import net from rupa` | TCP client/server |
| [sys](sys.md) | `import sys from rupa` | env, cwd, platform, arch, path utilities |
| [fs](fs.md) | `import fs from rupa` | read, write, append, exists, isDir |
| [database](database.md) | `import database from rupa` | penyedia koneksi driver (nosql, mariadb, psql, mysql, sqlite) |

Sintaks import/export lengkap: [Syntax — Import](../../syntax/import.md),
[Syntax — Export](../../syntax/export.md). Grammar implementasi:
[Grammar — Module](../../grammar/module.md).
