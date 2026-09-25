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

#endif
