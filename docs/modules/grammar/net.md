# Net Module Grammar

## AST Structure

### `net.connect(host, port)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: net
      Member:
        Identifier: connect
  Arg 1:
    String: "127.0.0.1"
  Arg 2:
    Number: 8080
```

### `net.accept(fd)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: net
      Member:
        Identifier: accept
  Arg 1:
    Identifier: server
```

## Module Structure

```text
net
├── connect(host, port)   — TCP connect
├── send(fd, data)        — send data
├── receive(fd, size?)    — receive data
├── close(fd)             — close socket
├── listen(port, backlog?) — TCP server
├── accept(fd)            — accept connection
└── resolve(host)         — DNS resolve
```
