# String Module Grammar

Module function-based dapat diakses melalui import:

```rupa
import string from rupa
string.upper(name)
```

Member module:

```text
string.length(value)
string.upper(value)
string.lower(value)
string.trim(value)
string.contains(value, search)
string.startsWith(value, prefix)
string.endsWith(value, suffix)
string.replace(value, old, new)
```

String juga mendukung member expression langsung:

```rupa
name.length
name.upper()
name.trim().lower()
```

`name.length` adalah member/property expression. `name.upper()` adalah call
expression dengan member expression sebagai callee; runtime mengikat receiver
`name` sebagai argumen native pertama.
