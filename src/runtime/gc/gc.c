#include <rupa.h>

/* gc.c — registry & alokasi inti GC.
 * Registry v4: parallel arrays {items, sizes, elems, types, vheads} plus
 * HASH INDEX ptr→slot (open addressing, no tombstone — penghapusan pakai
 * swap-delete sehingga rantai probe selalu utuh). Semua lookup O(1):
 * gcfind, gcsetsize/gcsetelem, gcfree/gcremove tidak lagi scan linear
 * seluruh registry (tests/stress/max_loop.rp: alokasi per-iterasi
 * O(n) → total O(n²)).
 * View (memory.c): header {owner, offset, prev, next} — rantai ganda
 * per-owner lewat vheads[slot]; gcfree me-NULL-kan owner view-nya dengan
 * O(jumlah view) — bukan scan seluruh registry.
 * Utilitas (dup family, memcpy/memset) ada di gc_extra.c. */

struct GarbageCollector *gc = NULL;

/* ===== Hash index ptr → slot ===== */

/* Slot kosong ditandai 0; entry menyimpan (slot + 1). Tanpa tombstone:
 * gcfree/gcremove memindahkan entri terakhir ke lubang (swap-delete),
 * jadi tabel hanya pernah INSERT dan UPDATE-slot. */
static int  *g_tab    = NULL; /* milik gc — dialokasi ulang saat grow */
static int   g_tcap   = 0;    /* panggku hash (power of two) */
static int   g_tcount = 0;    /* pemakaian tabel (== gc->count) */

static uint64_t gcHash(void *ptr) {
  /* Alokasi rata 16-byte aligned — buang bit rendah yang konstan,
   * Fibonacci multiplication + finalizer murmur3. */
  uint64_t x = (uint64_t)(uintptr_t)ptr >> 4;
  x *= 0x9E3779B97F4A7C15ULL;
  x ^= x >> 33;
  x *= 0xFF51AFD7ED558CCDULL;
  x ^= x >> 33;
  return x;
}

/* Cari slot registry `ptr` di tabel. Return slot (0..count-1) atau -1.
 * Dipanggil DENGAN lock terpegang. */
static int tabFind(void *ptr) {
  if (!g_tab || g_tcap == 0) return -1;
  uint64_t h = gcHash(ptr);
  int mask = g_tcap - 1;
  for (int i = (int)(h & mask); g_tab[i] != 0; i = (i + 1) & mask) {
    int slot = g_tab[i] - 1;
    if (slot >= 0 && slot < gc->count && gc->items[slot] == ptr) return slot;
  }
  return -1;
}

/* Sisipkan/update: ptr terdaftar di `slot`. Asumsi ptr belum ada di
 * tabel (pemanggil jalur append). Return false bila indeks tidak siap. */
static bool tabInsert(void *ptr, int slot) {
  if (!g_tab || g_tcap == 0) return false;
  uint64_t h = gcHash(ptr);
  int mask = g_tcap - 1;
  for (int i = (int)(h & mask);; i = (i + 1) & mask) {
    if (g_tab[i] == 0) {
      g_tab[i] = slot + 1;
      g_tcount++;
      return true;
    }
    int s = g_tab[i] - 1;
    if (s >= 0 && s < gc->count && gc->items[s] == ptr) {
      g_tab[i] = slot + 1; /* re-register ptr yang sama: perbarui slot */
      return true;
    }
  }
}

/* Perbarui slot index sebuah ptr yang SUDAH ada di tabel: cari entri
 * dengan nilai slot lama mulai dari hash-nya, tulis slot baru. */
static void tabUpdateSlot(uint64_t h, int old_slot, int new_slot) {
  if (!g_tab || g_tcap == 0) return;
  int mask = g_tcap - 1;
  for (int i = (int)(h & mask); g_tab[i] != 0; i = (i + 1) & mask) {
    if (g_tab[i] == old_slot + 1) {
      g_tab[i] = new_slot + 1;
      return;
    }
  }
}

/* Hapus entri tabel milik slot `slot`, probe mulai dari hash `h`.
 * `h` dihitung dari nilai pointer SEBELUM blok di-realloc/free.
 *
 * Linear probing TANPA tombstone: mengosongkan slot di tengah cluster
 * memutus rantai probe entri di belakangnya (lookup berhenti di lubang
 * — entri "hilang", gcfree selanjutnya salah). Maka pakai BACKSHIFT:
 * setiap entri cluster di belakang lubang yang masih terjangkau dari
 * posisi idealnya digeser mundur ke lubang; rantai tetap utuh. */
static void tabRemove(uint64_t h, int slot) {
  if (!g_tab || g_tcap == 0) return;
  int mask = g_tcap - 1;
  int i = (int)(h & mask);
  for (; g_tab[i] != 0; i = (i + 1) & mask) {
    if (g_tab[i] == slot + 1) break;
  }
  if (g_tab[i] == 0) return; /* tidak ditemukan */

  int hole = i;
  int j = (i + 1) & mask;
  while (g_tab[j] != 0) {
    int s = g_tab[j] - 1;
    if (s >= 0 && s < gc->count && gc->items[s]) {
      int ideal = (int)(gcHash(gc->items[s]) & mask);
      int d_hole = (hole - ideal + g_tcap) & mask;
      int d_j = (j - ideal + g_tcap) & mask;
      if (d_hole <= d_j) { /* hole terjangkau dari ideal entri ini */
        g_tab[hole] = g_tab[j];
        hole = j;
      }
    }
    j = (j + 1) & mask;
  }
  g_tab[hole] = 0;
  g_tcount--;
}

/* Bangun ulang tabel (rehash semua ptr yang hidup). Panggil saat load
 * factor tembus 3/4. */
static void tabRebuild(int new_cap) {
  free(g_tab);
  g_tab = calloc((size_t)new_cap, sizeof(int));
  if (!g_tab) {
    g_tcap = 0;
    g_tcount = 0;
    return;
  }
  g_tcap = new_cap;
  g_tcount = 0;
  for (int i = 0; i < gc->count; i++)
    if (gc->items[i]) tabInsert(gc->items[i], i);
}

/* ===== Forward ===== */
static void gcregmeta(void *ptr, size_t size, size_t elems);

/* ===== Lifecycle ===== */

void gcinit(int capacity) {
  if (capacity <= 0 || gc) return;

  gc = calloc(1, sizeof(GarbageCollector));
  if (!gc) return;

  gc->items = malloc(capacity * sizeof(void *));
  if (!gc->items) {
    free(gc);
    gc = NULL;
    return;
  }
  gc->sizes = calloc((size_t)capacity, sizeof(size_t));
  gc->elems = calloc((size_t)capacity, sizeof(size_t));
  gc->types = calloc((size_t)capacity, sizeof(char *));
  gc->vheads = calloc((size_t)capacity, sizeof(char *));
  if (!gc->sizes || !gc->elems || !gc->types || !gc->vheads) {
    free(gc->sizes);
    free(gc->elems);
    free(gc->types);
    free(gc->vheads);
    free(gc->items);
    free(gc);
    gc = NULL;
    return;
  }

  gc->capacity = capacity;
  gc->count = 0;

  int tcap = 1024;
  while (tcap < capacity * 2) tcap <<= 1;
  g_tab = calloc((size_t)tcap, sizeof(int));
  if (g_tab) {
    g_tcap = tcap;
    g_tcount = 0;
  }

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&gc->lock, &attr);
  pthread_mutexattr_destroy(&attr);
}

/* Tumbuhkan semua array paralel + rehash. Return false bila realloc
 * gagal (pemanggil memutuskan). */
static bool gcgrow(void) {
  int new_capacity = gc->capacity * 2;
  void **new_items = realloc(gc->items, new_capacity * sizeof(void *));
  if (!new_items) return false;
  size_t *new_sizes = realloc(gc->sizes, new_capacity * sizeof(size_t));
  size_t *new_elems = realloc(gc->elems, new_capacity * sizeof(size_t));
  char **new_types = realloc(gc->types, new_capacity * sizeof(char *));
  char **new_vheads = realloc(gc->vheads, new_capacity * sizeof(char *));
  if (!new_sizes || !new_elems || !new_types || !new_vheads) {
    /* realloc per-array tidak all-or-nothing; simpan yang berhasil —
     * kapasitas items adalah patokan dan itu pasti sudah diperbarui. */
    if (new_sizes) gc->sizes = new_sizes;
    if (new_elems) gc->elems = new_elems;
    if (new_types) gc->types = new_types;
    if (new_vheads) gc->vheads = new_vheads;
    return false;
  }
  gc->items = new_items;
  gc->sizes = new_sizes;
  gc->elems = new_elems;
  gc->types = new_types;
  gc->vheads = new_vheads;
  gc->capacity = new_capacity;

  if (g_tcap > 0 && (gc->count + 1) * 4 > g_tcap * 3) tabRebuild(g_tcap * 2);
  return true;
}

/* ===== Alokasi (jalur panas — O(1), tanpa gcfind) ===== */

/* Register blok baru sekaligus dengan size & elem — O(1) append.
 * Dipakai gcmall/gccalloc/gcrealloc agar jalur alokasi panas tidak
 * perlu gcfind (scan linear) setelahnya. */
static void gcregmeta(void *ptr, size_t size, size_t elems) {
  if (!gc || !ptr) return;

  pthread_mutex_lock(&gc->lock);
  if (gc->count >= gc->capacity) {
    gcgrow();
    if (gc->count >= gc->capacity) {
      pthread_mutex_unlock(&gc->lock);
      return;
    }
  }
  if (g_tcap == 0 || (gc->count + 1) * 4 > g_tcap * 3) {
    int want = g_tcap ? g_tcap * 2 : 1024;
    tabRebuild(want);
    if (g_tcap == 0) { /* rebuild gagal: registry tetap jalan tanpa indeks */
      gc->items[gc->count] = ptr;
      gc->sizes[gc->count] = size;
      gc->elems[gc->count] = elems;
      gc->types[gc->count] = NULL;
      gc->vheads[gc->count] = NULL;
      gc->count++;
      pthread_mutex_unlock(&gc->lock);
      return;
    }
  }
  gc->items[gc->count] = ptr;
  gc->sizes[gc->count] = size;
  gc->elems[gc->count] = elems;
  gc->types[gc->count] = NULL;
  gc->vheads[gc->count] = NULL;
  tabInsert(ptr, gc->count);
  gc->count++;
  pthread_mutex_unlock(&gc->lock);
}

void gcreg(void *ptr) {
  if (!gc || !ptr) return;
  gcregmeta(ptr, 0, 0);
}

void *gcmall(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) return NULL;
  if (!gc) return ptr;
  gcregmeta(ptr, size, 0);
  return ptr;
}

void *gccalloc(size_t num, size_t size) {
  /* calloc: zeroing sekaligus overflow-check (glibc) — menggantikan
   * pola lama gcmall+memset yang butuh gcfind linear tambahan. */
  void *ptr = calloc(num ? num : 1, size ? size : 1);
  if (!ptr) return NULL;
  if (!gc) return ptr;
  gcregmeta(ptr, num * size, num);
  return ptr;
}

void *gcrealloc(void *ptr, size_t new_size) {
  if (!gc) return realloc(ptr, new_size);

  pthread_mutex_lock(&gc->lock);
  int index = tabFind(ptr);
  /* Hash dari nilai pointer lama — dihitung SEBELUM realloc supaya
   * tidak menyentuh nilai pointer yang sudah tidak valid. */
  uint64_t old_hash = (index >= 0) ? gcHash(ptr) : 0;
  void *new_ptr = realloc(ptr, new_size);
  if (new_ptr) {
    if (index >= 0) {
      /* Registry v2: ukuran mengikuti realloc; elemen tidak diketahui
       * di sini (pemanggil setel via gcsetelem bila relevan). */
      if (new_ptr != ptr) {
        tabRemove(old_hash, index);
        gc->items[index] = new_ptr;
        tabInsert(new_ptr, index);
      }
      gc->sizes[index] = new_size;
    } else {
      gcregmeta(new_ptr, new_size, 0);
    }
  }
  pthread_mutex_unlock(&gc->lock);
  return new_ptr;
}

void *gcarray(void *ptr, size_t count, size_t element_size) {
  /* Semantik reallocarray penuh: tolak jika count * element_size
   * overflow alih-alih diam-diam mengalokasikan kekurangan.
   * ptr == NULL -> alokasi baru (terdaftar), selain itu resize
   * dengan memperbarui registry (via gcrealloc). */
  if (count != 0 && element_size > ((size_t)-1) / count) return NULL;
  /* gcrealloc sudah memutakhirkan size (registered maupun baru); yang
   * tersisa hanya elem. */
  void *result = gcrealloc(ptr, count * element_size);
  if (result) gcsetelem(result, count);
  return result;
}

/* ===== Pencabutan (swap-delete — O(1) + O(views owner)) ===== */

/* Lepas slot `i` dari registry: entri terakhir dipindah ke `i`
 * (swap-delete) sehingga tidak ada lubang/tombstone di tabel hash.
 * Harus dipanggil DENGAN lock terpegang. */
/* Lepas slot `i` dari registry: entri terakhir dipindah ke `i`
 * (swap-delete) sehingga tidak ada lubang/tombstone di tabel hash.
 * `h` = gcHash(ptr) — dihitung PEMANGGIL sebelum blok di-free supaya
 * tidak menyentuh pointer yang sudah tidak valid.
 * Harus dipanggil DENGAN lock terpegang. */
static void gcDetachSlot(uint64_t h, int i) {
  int last = gc->count - 1;

  tabRemove(h, i);

  if (i != last) {
    void *moved = gc->items[last];
    gc->items[i] = moved;
    gc->sizes[i] = gc->sizes[last];
    gc->elems[i] = gc->elems[last];
    gc->types[i] = gc->types[last];
    gc->vheads[i] = gc->vheads[last];
    tabUpdateSlot(gcHash(moved), last, i);
  }
  gc->count--;
}

void gcfree(void *ptr) {
  if (!gc || !ptr) {
    free(ptr);
    return;
  }

  pthread_mutex_lock(&gc->lock);
  int i = tabFind(ptr);
  if (i < 0) {
    pthread_mutex_unlock(&gc->lock);
    free(ptr);
    return;
  }

  if (gc->types[i]) {
    free(gc->types[i]);
    gc->types[i] = NULL;
  }

  if (gc->elems[i] == GC_VIEW_MAGIC) {
    /* Yang di-free adalah VIEW: cabut diri dari rantai owner-nya. */
    void *owner = NULL;
    memcpy(&owner, ptr, sizeof(owner));
    if (owner) {
      int os = tabFind(owner);
      if (os >= 0) {
        char *vprev = NULL, *vnext = NULL;
        memcpy(&vprev, (char *)ptr + 2 * sizeof(void *), sizeof(vprev));
        memcpy(&vnext, (char *)ptr + 3 * sizeof(void *), sizeof(vnext));
        if (vprev) {
          memcpy((char *)vprev + 3 * sizeof(void *), &vnext, sizeof(vnext));
        } else if (gc->vheads[os] == (char *)ptr) {
          gc->vheads[os] = vnext;
        }
        if (vnext) {
          memcpy((char *)vnext + 2 * sizeof(void *), &vprev, sizeof(vprev));
        }
      }
    }
    /* View sendiri tidak punya sub-view via vheads (gcregview selalu
     * resolve view ke blok asli sebelum membuat view baru) — tapi
     * tetap bersihkan supaya konsisten. */
    gc->vheads[i] = NULL;
  } else if (gc->vheads[i]) {
    /* Owner block: NULL-kan header owner semua view-nya — akses lewat
     * view setelahnya tertangkap guard (bukan UAF). O(jumlah view). */
    for (char *v = gc->vheads[i]; v;) {
      void *zero = NULL;
      memcpy(v, &zero, sizeof(zero));
      memcpy(&v, v + 3 * sizeof(void *), sizeof(v)); /* next */
    }
    gc->vheads[i] = NULL;
  }

  uint64_t h = gcHash(ptr);
  free(ptr);
  gcDetachSlot(h, i);
  pthread_mutex_unlock(&gc->lock);
}

void gcremove(void *ptr) {
  if (!gc || !ptr) return;

  pthread_mutex_lock(&gc->lock);
  int i = tabFind(ptr);
  if (i < 0) {
    pthread_mutex_unlock(&gc->lock);
    return;
  }
  if (gc->types[i]) {
    free(gc->types[i]);
    gc->types[i] = NULL;
  }
  /* Perilaku lama dipertahankan: gcremove hanya mencabut registrasi —
   * memori tetap hidup, view tidak di-NULL-kan. */
  gc->vheads[i] = NULL;
  gcDetachSlot(gcHash(ptr), i);
  pthread_mutex_unlock(&gc->lock);
}

/* ===== View handle (memory.c) ===== */

/* View = blok GC kecil (diregister normal): header 4 word
 * {owner, offset, prev, next} — rantai ganda seluruh view milik satu
 * owner, kepala rantai di gc->vheads[slot owner]. Bukan mekanisme
 * registry baru — view adalah item biasa dengan elems == GC_VIEW_MAGIC
 * sehingga gcsize/gcelem/gcfree bekerja tanpa jalur khusus; gcfree
 * me-NULL-kan owner view yang menunjuk blok yang dibebaskan → akses
 * setelahnya tertangkap guard, bukan UAF. */
void *gcregview(void *owner, size_t offset) {
  if (!gc || !owner) return NULL;
  pthread_mutex_lock(&gc->lock);
  /* owner boleh sendiri view (nested: o.inner.x, arr[i].inner.y) —
   * resolve rantai ke blok asli, offset dijumlahkan. */
  for (int hop = 0; hop < 16; hop++) {
    int oi = tabFind(owner);
    if (oi < 0) {
      pthread_mutex_unlock(&gc->lock);
      return NULL;
    }
    if (gc->elems[oi] != GC_VIEW_MAGIC) break;
    size_t vowner;
    size_t voffset;
    memcpy(&vowner, owner, sizeof(vowner));
    memcpy(&voffset, (char *)owner + sizeof(vowner), sizeof(voffset));
    if (!vowner) {
      pthread_mutex_unlock(&gc->lock);
      return NULL; /* view dangling (owner sudah di-free) */
    }
    owner = (void *)vowner;
    offset = (size_t)voffset + offset;
  }
  int slot = tabFind(owner);
  if (slot < 0) {
    pthread_mutex_unlock(&gc->lock);
    return NULL;
  }
  /* Stale view dalam rantai (owner dangling) — daun rantai lama tetap
   * ter-link; header baru menaut ke kepala rantai sekarang. */
  char *head = gc->vheads[slot];

  /* Alokasi di luar lock: gcmall reentry ke registry dengan lock-nya
   * sendiri (recursive). Header view: {owner, offset, prev, next}. */
  void **header = calloc(4, sizeof(void *));
  if (!header) {
    pthread_mutex_unlock(&gc->lock);
    return NULL;
  }
  header[0] = owner;
  header[1] = (void *)offset;
  header[2] = NULL;    /* prev */
  header[3] = (void *)head; /* next */
  gcregmeta(header, 4 * sizeof(void *), GC_VIEW_MAGIC);
  int vslot = tabFind(header);
  if (vslot < 0) {
    free(header);
    pthread_mutex_unlock(&gc->lock);
    return NULL;
  }
  if (head) memcpy((char *)head + 2 * sizeof(void *), &header, sizeof(header));
  gc->vheads[slot] = (char *)header;
  pthread_mutex_unlock(&gc->lock);
  return (void *)header;
}

/* addr view handle? Return blok pemiliknya (NULL bila addr bukan view
 * / view dangling). */
void *gcregisview(void *addr) {
  if (!gc || !addr || tabFind(addr) < 0) return NULL;
  if (gcelem(addr) != GC_VIEW_MAGIC) return NULL;
  void *owner = NULL;
  memcpy(&owner, addr, sizeof(owner));
  return owner;
}

/* ===== Lookup publik ===== */

int gcfind(void *ptr) {
  if (!gc || !ptr) return -1;

  /* Called both with and without lock held — index hanya dimutasi di
   * bawah lock, pembacaan slot aman (int tunggal). */
  pthread_mutex_lock(&gc->lock);
  int result = tabFind(ptr);
  pthread_mutex_unlock(&gc->lock);
  return result;
}

/* ===== Registry v3: nama tipe elemen ===== */

const char *gcregtype(void *ptr) {
  if (!gc || !ptr) return NULL;
  pthread_mutex_lock(&gc->lock);
  int index = tabFind(ptr);
  const char *type = index >= 0 ? gc->types[index] : NULL;
  pthread_mutex_unlock(&gc->lock);
  return type;
}

bool gcregsettype(void *ptr, const char *type) {
  if (!gc || !ptr || !type) return false;
  pthread_mutex_lock(&gc->lock);
  int index = tabFind(ptr);
  if (index < 0) {
    pthread_mutex_unlock(&gc->lock);
    return false;
  }
  if (gc->types[index]) free(gc->types[index]);
  /* Metadata registry dikelola MANUAL (bukan gcstrdup — gcstrdup
   * meregestrasi hasilnya juga, dan buffer yang sama akan di-free
   * dua kali: sebagai items[] dan sebagai types[]). */
  gc->types[index] = strdup(type);
  pthread_mutex_unlock(&gc->lock);
  return gc->types[index] != NULL;
}

/* ===== Registry v2: ukuran blok & elemen ===== */

size_t gcsize(void *ptr) {
  if (!gc || !ptr) return 0;
  pthread_mutex_lock(&gc->lock);
  int index = tabFind(ptr);
  size_t size = index >= 0 ? gc->sizes[index] : 0;
  pthread_mutex_unlock(&gc->lock);
  return size;
}

size_t gcelem(void *ptr) {
  if (!gc || !ptr) return 0;
  pthread_mutex_lock(&gc->lock);
  int index = tabFind(ptr);
  size_t elem = index >= 0 ? gc->elems[index] : 0;
  pthread_mutex_unlock(&gc->lock);
  return elem;
}

void gcsetsize(void *ptr, size_t size) {
  if (!gc || !ptr) return;
  pthread_mutex_lock(&gc->lock);
  int index = tabFind(ptr);
  if (index >= 0) gc->sizes[index] = size;
  pthread_mutex_unlock(&gc->lock);
}

void gcsetelem(void *ptr, size_t count) {
  if (!gc || !ptr) return;
  pthread_mutex_lock(&gc->lock);
  int index = tabFind(ptr);
  if (index >= 0) gc->elems[index] = count;
  pthread_mutex_unlock(&gc->lock);
}

void *gcresize(void *ptr, size_t old_size, size_t new_size) {
  void *new_ptr = gcrealloc(ptr, new_size);
  if (new_ptr && new_size > old_size) memset((char *)new_ptr + old_size, 0, new_size - old_size);
  return new_ptr;
}

/* ===== Shutdown ===== */

void gcclean(void) {
  if (!gc) return;

  pthread_mutex_lock(&gc->lock);
  if (gc->count > 0 && gc->items) {
    for (int i = 0; i < gc->count; i++) {
      if (gc->items[i]) {
        free(gc->items[i]);
        gc->items[i] = NULL;
      }
      if (gc->types && gc->types[i]) {
        free(gc->types[i]);
        gc->types[i] = NULL;
      }
    }
    free(gc->items);
    gc->items = NULL;
  }
  if (gc->sizes) free(gc->sizes);
  if (gc->elems) free(gc->elems);
  if (gc->types) free(gc->types);
  if (gc->vheads) free(gc->vheads);
  pthread_mutex_unlock(&gc->lock);

  free(g_tab);
  g_tab = NULL;
  g_tcap = 0;
  g_tcount = 0;

  pthread_mutex_destroy(&gc->lock);
  free(gc);
  gc = NULL;
}
