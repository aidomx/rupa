#pragma once

#if defined(RUPA_PACKAGE_H)

/* ============================================================
 * expr.h — memo parse per-produksi (level ekspresi)
 *
 * Dua span token dengan KONTEN sama (mis. `arr[i]` yang muncul
 * sepuluh kali, atau `i < 10` pada dua loop) di-parse SEKALI —
 * node id hasil parse dipakai bersama. Milik satu generasi node
 * pool (Request): pointer node AST antar file tidak saling
 * terpakai, maka memo di-reset tiap Request baru.
 *
 * Terhubung via field opaque di struct Request; alokasi memakai
 * GC arena (gcalloc/gcfree) — hidup sampai gcclean(), murah.
 * ============================================================ */

/* Lookup memo: span [a,b) dengan konten sama pernah di-parse?
 * Hit: *outNodeId diisi, return true (caller PAKAI node tanpa
 * parse ulang — parsing menghasilkan node yang setara). Miss:
 * return false. */
bool syntaxMemoLookup(struct Request *r, int a, int b, int *outNodeId);

/* Simpan hasil parse span [a,b) → nodeId (dipanggil SETELAH
 * parse sukses, id >= 0). Penyimpanan diam-diam dilewati bila
 * arena penuh/alokasi gagal. */
void syntaxMemoStore(struct Request *r, int a, int b, int nodeId);

/* Lepas memo milik Request (tanpa free node — node pool dimiliki
 * Request). Dipanggil saat Request dihancurkan. */
void syntaxMemoDetach(struct Request *r);

/* Statistik hit/miss sejak proses start (diagnostik). */
int syntaxMemoHits(void);
int syntaxMemoMisses(void);

#endif
