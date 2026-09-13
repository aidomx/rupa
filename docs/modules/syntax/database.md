# Database

Modul `database` adalah penyedia koneksi ke driver database: `nosql`, `mariadb`, `mysql`, `psql`, `sqlite`. Ditulis murni dengan bahasa Rupa.

```rupa
import db from rupa
```

## Registry driver

```rupa
print(db.driver.list())            // ["nosql", "mariadb", "mysql", "psql", "sqlite"]
print(db.driver.has("sqlite"))     // true
print(db.driver.get("psql"))       // spec driver: port default, kebutuhan auth
```

## DSN

Bentuk umum DSN:

```
scheme://[user[:pass]@]host[:port]/database
```

Contoh:

```
mariadb://admin:secret@db.local:3307/shop
psql://user:pass@localhost/app
nosql://cluster.local:9200/cache
sqlite://data/app.db
sqlite:///absolute/path/file.db
```

Parser DSN tersedia jika ingin mengurai manual:

```rupa
u = db.dsn.parse("mariadb://admin:secret@db.local:3307/shop")
print(u.host)   // db.local
print(u.port)   // 3307
print(u.user)   // admin
print(u.path)   // /shop
```

## Membuka koneksi

```rupa
c = db.open("mariadb://admin:secret@db.local/shop")
if c.ok:
    print(c.driver)     // mariadb
    print(c.host)       // db.local
    print(c.port)       // 3306 (default dari spec driver)
    print(c.database)   // shop
    print(c.status)     // ready
else:
    print(c.error)
```

Khusus `sqlite`, authority + path adalah lokasi file:

```rupa
c = db.open("sqlite://data/app.db")
print(c.database)  // data/app.db
```

## Validasi

`db.open()` memvalidasi DSN terhadap spec driver dan mengembalikan object dengan `ok: false` beserta `error` bila tidak valid — tidak pernah melempar exception:

```rupa
print(db.open("oracle://x/y").error)
// unknown driver: oracle

print(db.open("mariadb://admin@db.local/shop").error)
// user and password are required
```

## Status

Saat ini `db.open()` belum melakukan I/O jaringan. Status `ready` berarti DSN valid dan parameter koneksi lengkap — siap disambungkan ke implementasi driver nyata.
