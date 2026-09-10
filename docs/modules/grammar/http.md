# HTTP Module Grammar

Module `http` diimpor dari root namespace Rupa:

```rupa
import http from rupa
```

## AST Structure

### `http.server(port, handler?)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: http
      Member:
        Identifier: server
  Arg 1:
    Number: 8080
```

### `http.stop(handle)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: http
      Member:
        Identifier: stop
  Arg 1:
    Identifier: server
```

### `http.request(method, url, data?)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: http
      Member:
        Identifier: request
  Arg 1:
    String: "GET"
  Arg 2:
    String: "https://api.example.com/data"
```

### `http.get(url)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: http
      Member:
        Identifier: get
  Arg 1:
    String: "https://api.example.com/data"
```

### `http.post(url, data)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: http
      Member:
        Identifier: post
  Arg 1:
    String: "https://api.example.com/data"
  Arg 2:
    String: "name=Rupa"
```

## Module Structure

```text
http
├── server(port, handler?) — start HTTP server
├── stop(handle)           — stop server
├── request(method, url, data?) — HTTP client
├── get(url)               — convenience GET
├── post(url, data)        — convenience POST
├── put(url, data)         — convenience PUT
├── delete(url)            — convenience DELETE
└── patch(url, data)       — convenience PATCH
```

## Implementation Notes

- Server uses POSIX sockets (socket, bind, listen, accept)
- Client uses curl (must be installed)
- Server runs in background thread
- Handler called on main thread (thread-safe queue design)
- Request object: req.method, req.path, req.headers, req.body
- Response object: res.status, res.body, res.setHeader(), res.json()
