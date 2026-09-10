# HTTP Grammar

Grammar HTTP module mendefinisikan struktur AST untuk HTTP server dan client.

## Module Structure

HTTP module adalah C stdlib yang diakses melalui `import http from rupa`.

## HTTP Server

### `http.server(port)`

```rupa
server = http.server(8080)
```

AST:

```text
Assignment:
  Target:
    Identifier: server
  Value:
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

```rupa
http.stop(server)
```

AST:

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

## HTTP Client

### `http.request(method, url, data?)`

```rupa
response = http.request("GET", "https://api.example.com/data")
```

AST:

```text
Assignment:
  Target:
    Identifier: response
  Value:
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

```rupa
response = http.get("https://api.example.com/data")
```

AST:

```text
Assignment:
  Target:
    Identifier: response
  Value:
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

```rupa
response = http.post("https://api.example.com/data", "name=Rupa")
```

AST:

```text
Assignment:
  Target:
    Identifier: response
  Value:
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

## Function Import

Fungsi bisa diimport langsung:

```rupa
import server, stop, get, post from rupa.http

s = server(8080)
stop(s)
```

AST:

```text
Module statement:
  ArrayLiteral:
    Literal ID: server
    Literal ID: stop
    Literal ID: get
    Literal ID: post
  from:
    Literal ID: rupa.http
```

## HTTP Structure

```text
http
├── server(port) -> handle
├── stop(handle)
├── request(method, url, data?) -> response
├── get(url) -> response
├── post(url, data) -> response
├── put(url, data) -> response
├── delete(url) -> response
└── patch(url, data) -> response
```

## Status Codes

Response mengembalikan string body. Status code belum tersedia di versi ini.

## Implementation Details

- Server menggunakan POSIX sockets (`socket`, `bind`, `listen`, `accept`)
- Client menggunakan `curl` atau `wget` (harus terinstall)
- Server berjalan di background thread
- Timeout default: 30 detik
