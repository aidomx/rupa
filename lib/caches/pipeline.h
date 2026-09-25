#pragma once

#if defined(RUPA_PACKAGE_H)

/* ============================================================
 * pipeline.h — cache level pipeline (AST + IR)
 *
 * Kunci = path + mtime + mtime_nsec + size (pola FileCache di
 * prompt/runner.c). Nilai = Node* hasil processGenerate dan
 * IRModule* hasil rewrite.
 *
 * Kepemilikan: entry yang tersimpan DIMILIKI cache. pipelineCachePut()
 * MEMINDAHKAN kepemilikan (node, ir) ke cache — caller tidak boleh
 * memanggil irModuleFree/clearNode untuk pointer yang sudah di-put.
 *
 * Keamanan memori: alokasi AST/IR semuanya lewat GC arena (gcstrdup/
 * gccalloc) dan gcclean() hanya dipanggil di ujung dispatch
 * (bootstrap/loader.c) — bukan antar file — sehingga pointer yang
 * dicache tetap valid sepanjang proses. pipelineCacheReset() wajib
 * dipanggil sebelum gcclean() bila proses berlanjut setelahnya.
 * ============================================================ */

/* Cari entry cache untuk path. Metadata disk (mtime+nsec+size)
 * divalidasi ulang tiap panggilan; entry basi (file berubah) diusir.
 * Hit: *outNode dan *outIr diisi (outIr boleh NULL untuk entry AST-only)
 * dan return true. */
bool pipelineCacheGet(const char *path, Node **outNode, IRModule **outIr);

/* Isi/refresh cache: memindahkan kepemilikan (node, ir) ke cache.
 * ir boleh NULL (kategori yang tidak membangun IR). Setelah put
 * sukses, caller kehilangan hak atas kedua pointer tersebut. */
void pipelineCachePut(const char *path, Node *node, IRModule *ir);

/* Buang seluruh index (pointer TIDAK di-free — GC arena melepasnya
 * di gcclean). Dipanggil sebelum gcclean bila proses berlanjut. */
void pipelineCacheReset(void);

/* Statistik hit/miss sejak reset terakhir (diagnostik). */
int pipelineCacheHits(void);
int pipelineCacheMisses(void);

#endif
