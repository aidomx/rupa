# HTTP Module

Modul HTTP menyediakan client dan server HTTP sederhana.

```rupa
import http from rupa
```

## `http.server(port, handler?)`

Memulai HTTP server pada port tertentu. Mengembalikan server handle.

### Tanpa handler (default response)

```rupa
import http from rupa

server = http.server(8080)
print("Server started on port 8080")
```

### Dengan handler

Handler menerima objek `req` (request) dan `res` (response):

```rupa
import http from rupa

handler(req, res) {
  res.body = "Hello " + req.path + "!"
  return res
}

server = http.server(8080, handler)
```

**Request object:**
- `req.method` — HTTP method (GET, POST, dll)
- `req.path` — URL path
- `req.headers` — raw headers string
- `req.body` — request body

**Response object:**
- `res.status` — status code (default: 200)
- `res.body` — response body
- `res.setHeader(key, value)` — set response header
- `res.json(obj)` — serialize objek ke JSON

### Contoh handler lengkap

```rupa
import http from rupa

handler(req, res) {
  res.setHeader("X-Server", "rupa")

  if req.path == "/api/data":
    res.json({ status: "ok", data: [1, 2, 3] })
  else:
    res.body = "Welcome to Rupa HTTP Server!"

  return res
}

server = http.server(8080, handler)
```

## `http.stop(handle)`

Menghentikan server yang sedang berjalan.

```rupa
http.stop(server)
print("Server stopped")
```

## `http.request(method, url, data?)`

Melakukan HTTP request. Method: `"GET"`, `"POST"`, `"PUT"`, `"DELETE"`.

```rupa
import http from rupa

// GET request
response = http.request("GET", "https://api.example.com/data")
print(response)

// POST request
response = http.request("POST", "https://api.example.com/data", "name=Rupa")
print(response)
```

## `http.get(url)`

Shortcut untuk GET request.

```rupa
response = http.get("https://api.example.com/data")
print(response)
```

## `http.post(url, data)`

Shortcut untuk POST request.

```rupa
response = http.post("https://api.example.com/data", "name=Rupa")
print(response)
```

## Contoh: HTTP Server Sederhana

```rupa
import http from rupa
import thread from rupa

// Start server
server = http.server(8080)
print("Server running on http://localhost:8080")

// Keep server running
thread.sleep(5000)

// Stop server
http.stop(server)
print("Server stopped")
```

## Contoh: HTTP Client

```rupa
import http from rupa

// Fetch data from API
response = http.get("https://jsonplaceholder.typicode.com/todos/1")
print(response)

// Send data
response = http.post("https://jsonplaceholder.typicode.com/posts", "title=foo&body=bar&userId=1")
print(response)
```

## Catatan

- Server berjalan di background thread
- Client menggunakan `curl` atau `wget` (harus terinstall)
- Response berupa string (body response)
- Timeout default: 30 detik
