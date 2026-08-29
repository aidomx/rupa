# Os Module Grammar

Modul `os` adalah bagian dari standard library (`rupa`) yang menyediakan akses ke sistem operasi.

## Pernyataan Import

```text
import os from rupa
```

Parser membuat node:

```text
NODE_IMPORT
└── module.value → NODE_LITERAL_ID ("os")
```

Interpreter menginisialisasi modul `os` dan mendaftarkan semua fungsi native ke dalam environment.

## Struktur Module Object

Modul `os` dikembalikan sebagai `VALUE_OBJECT` dengan entri berikut:

```text
os (VALUE_OBJECT)
├── exec     → VALUE_NATIVE_FUNCTION (1 param)
├── getcwd   → VALUE_NATIVE_FUNCTION (0 param)
├── exit     → VALUE_NATIVE_FUNCTION (1 param)
├── getenv   → VALUE_NATIVE_FUNCTION (1 param)
├── chdir    → VALUE_NATIVE_FUNCTION (1 param)
├── mkdir    → VALUE_NATIVE_FUNCTION (1 param)
├── remove   → VALUE_NATIVE_FUNCTION (1 param)
├── rename   → VALUE_NATIVE_FUNCTION (2 param)
├── listdir  → VALUE_NATIVE_FUNCTION (1 param)
├── info     → VALUE_NATIVE_FUNCTION (1 param, optional)
└── path     → VALUE_OBJECT
    └── exists → VALUE_NATIVE_FUNCTION (1 param)
```

## Tipe Value Native Function

Fungsi native menggunakan tipe `VALUE_NATIVE_FUNCTION` dengan struktur:

```text
struct RuntimeNativeFunction {
  const char *name;
  NativeFn func;        // function pointer
  int paramCount;
};
```

Signature fungsi native:

```c
typedef InterpreterResult (*NativeFn)(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error);
```

## AST Node Types

Modul mendefinisikan node types berikut:

```text
NODE_MODULE
├── NODE_IMPORT
├── NODE_EXPORT
└── NODE_EXTENDS
```

## Penanganan Import

Interpreter menangani `NODE_IMPORT` di `interpretNode()`:

```text
1. Ekstrak nama modul dari expression (NODE_LITERAL_ID)
2. Cari modul di registry stdlib
3. Jika ditemukan, register ke environment dengan semSet()
```

## Dependency

- `<dirent.h>` - untuk `listdir()`
- `<sys/utsname.h>` - untuk `info()`
- `<unistd.h>` - untuk `chdir()`, `getcwd()`, `gethostname()`
- `<sys/stat.h>` - untuk `mkdir()`, `path.exists()`
