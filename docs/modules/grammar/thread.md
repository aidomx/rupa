# Thread Module Grammar

Module `thread` diimpor dari root namespace Rupa:

```rupa
import thread from rupa
```

## AST Structure

### `thread.sleep(ms)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: thread
      Member:
        Identifier: sleep
  Arg 1:
    Number: 1000
```

### `thread.id()`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: thread
      Member:
        Identifier: id
```

### `thread.count()`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: thread
      Member:
        Identifier: count
```

### `thread.create(fn)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: thread
      Member:
        Identifier: create
  Arg 1:
    Identifier: fn
```

### `thread.join(handle)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: thread
      Member:
        Identifier: join
  Arg 1:
    Identifier: handle
```

## Module Structure

```text
Module: thread
├── sleep(ms)    — nanosleep wrapper
├── id()         — pthread_self hash
├── count()      — active thread count
├── create(fn)   — pthread_create wrapper
└── join(handle) — pthread_join wrapper
```

## Implementation Notes

- Thread state stored in static `threadTable[MAX_THREADS]`
- `thread.create()` accepts `VALUE_NATIVE_FUNCTION` only
- `thread.join()` blocks until thread completes
- Thread results stored in `ThreadEntry.result`
- Error handling via `ThreadEntry.has_error` and `error_msg`
