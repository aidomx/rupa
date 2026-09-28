#pragma once

#if defined(RUPA_PACKAGE_H)

/* ============================================================
 * canonical.h — pool subtree AST kanonik lintas Request
 *
 * Melengkapi memo per-Request (expr.h): subtree ekspresi murni
 * dipublikasikan sekali; file lain dengan konten token identik
 * meng-adopt-nya (deep copy + remap id ke pool sendiri) tanpa
 * mem-parse ulang. Yang dibagi antar file hanya DATA subtree —
 * id file asal tidak pernah dipakai ulang.
 * ============================================================ */

/* Publikasikan subtree root (ekspresi murni, allowlist tipe) dari
 * pool req untuk dipakai lintas file. Diam-diam dilewati bila
 * subtree memuat tipe non-ekspresi atau melebihi batas. */
void syntaxCanonicalPublish(struct Request *req, int spanA, int spanB, int rootId);

/* Cari subtree ber-konten token identik dengan span [spanA,spanB).
 * Hit: subtree di-copy ke pool req (id baru), return id root.
 * Miss: return -1. Verifikasi token-per-token (dual-hash hanya
 * penyaring). */
int syntaxCanonicalAdopt(struct Request *req, int spanA, int spanB);

/* Kosongkan index (alokasi tetap di GC arena sampai gcclean). */
void syntaxCanonicalReset(void);

/* Statistik sejak reset terakhir (diagnostik). */
int syntaxCanonicalHits(void);
int syntaxCanonicalMisses(void);

/* Jumlah entri pool saat ini (mode `rupa profile --cache`). */
int syntaxCanonicalCount(void);

/* ---- Diagnostik `rupa profile --optimizer` ----
 * Perekaman kegiatan pool (publish/adopt) untuk deteksi duplikasi
 * pekerjaan lintas file: entri yang di-adopt berarti ekspresi yang
 * sama sudah pernah di-parse di file lain. */

typedef enum {
  CANON_EVENT_PUBLISH = 0, /* entri baru dipublikasikan (kemunculan pertama) */
  CANON_EVENT_ADOPT = 1    /* entri lama di-adopt (duplikat — tanpa parse ulang) */
} SyntaxCanonEventKind;

typedef struct {
  SyntaxCanonEventKind kind;
  int entry; /* index entri pool terkait */
  int line;  /* baris span di file pemanggil (1-based) */
} SyntaxCanonEvent;

/* Aktifkan/nonaktifkan perekaman event. Buffer event dibatasi; ketika
 * penuh, event baru dibuang (flag overflow). */
void syntaxCanonicalTraceEnable(bool on);

/* Ambil event sejak pengambilan terakhir (maks `max`). Return jumlah
 * yang dikopi; buffer dikosongkan setelahnya. */
int syntaxCanonicalTakeEvents(SyntaxCanonEvent *out, int max);

/* Rekonstruksi teks kasar entri (join value stream dengan spasi).
 * Return panjang teks, -1 bila entry tidak valid. */
int syntaxCanonicalEntryText(int entry, char *out, int max);

#endif
