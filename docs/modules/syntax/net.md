# Net Module

Module `net` menyediakan fungsi networking (TCP socket).

```rupa
import net from rupa
```

## Fungsi

### net.connect(host, port)
Buat koneksi TCP ke host:port. Mengembalikan file descriptor.

```rupa
fd = net.connect("127.0.0.1", 8080)
```

### net.send(fd, data)
Kirim data melalui socket.

```rupa
net.send(fd, "Hello Server!")
```

### net.receive(fd, size?)
Terima data dari socket. Default buffer: 4096 bytes.

```rupa
data = net.receive(fd)
data = net.receive(fd, 1024)
```

### net.close(fd)
Tutup socket.

```rupa
net.close(fd)
```

### net.listen(port, backlog?)
Buat TCP server socket. Mengembalikan file descriptor.

```rupa
server = net.listen(8080)
server = net.listen(8080, 10)  // backlog 10
```

### net.accept(fd)
Terima koneksi baru. Mengembalikan `{fd, address, port}`.

```rupa
client = net.accept(server)
print(client.fd)
print(client.address)
print(client.port)
```

### net.resolve(host)
Resolve hostname ke IP address.

```rupa
ip = net.resolve("localhost")  // 127.0.0.1
ip = net.resolve("example.com")
```

| Fungsi | Parameter | Return |
|--------|-----------|--------|
| `connect(host, port)` | string, number | number (fd) |
| `send(fd, data)` | number, string | number (bytes sent) |
| `receive(fd, size?)` | number, number? | string |
| `close(fd)` | number | boolean |
| `listen(port, backlog?)` | number, number? | number (fd) |
| `accept(fd)` | number | object {fd, address, port} |
| `resolve(host)` | string | string (IP) |

## Contoh: Simple Server

```rupa
import net from rupa

server = net.listen(8080)
print("Listening on port 8080\n")

while true {
  client = net.accept(server)
  data = net.receive(client.fd)
  net.send(client.fd, "HTTP/1.1 200 OK\r\n\r\nHello!")
  net.close(client.fd)
}
```
