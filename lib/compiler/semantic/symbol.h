#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Membuat environment scope baru.
 *
 * @param parent Parent scope (NULL untuk global scope).
 * @return Pointer ke RuntimeEnv yang baru, atau NULL jika gagal.
 */
RuntimeEnv *semCreateEnv(RuntimeEnv *parent);

/**
 * @brief Mencari binding lokal dalam satu scope.
 *
 * @param env Environment scope.
 * @param name Nama variabel.
 * @return Pointer ke RuntimeBinding, atau NULL jika tidak ditemukan.
 */
RuntimeBinding *semFindLocal(RuntimeEnv *env, const char *name);

/**
 * @brief Mendeklarasikan variabel baru dalam scope.
 *
 * Jika variabel sudah ada di scope lokal, update tipenya saja.
 *
 * @param env Environment scope.
 * @param name Nama variabel.
 * @param type Tipe variabel (opsional, bisa NULL).
 * @return true jika berhasil.
 */
bool semDeclare(RuntimeEnv *env, const char *name, const char *type);

/**
 * @brief Mengatur nilai variabel dalam scope.
 *
 * Jika variabel belum ada, mendeklarasikannya terlebih dahulu.
 *
 * @param env Environment scope.
 * @param name Nama variabel.
 * @param value Nilai runtime.
 */
void semSet(RuntimeEnv *env, const char *name, RuntimeValue value);

/**
 * @brief Mendapatkan nilai variabel dari scope atau parent scope.
 *
 * @param env Environment scope (akan traverse ke parent jika perlu).
 * @param name Nama variabel.
 * @param out Pointer ke RuntimeValue untuk menyimpan hasil.
 * @return true jika variabel ditemukan.
 */
bool semGet(RuntimeEnv *env, const char *name, RuntimeValue *out);

/**
 * @brief Mendapatkan tipe variabel dari scope atau parent scope.
 *
 * @param env Environment scope.
 * @param name Nama variabel.
 * @return String tipe, atau NULL jika tidak ditemukan.
 */
const char *semType(RuntimeEnv *env, const char *name);

/**
 * @brief Mencari binding di seluruh scope chain (termasuk parent).
 *
 * @param env Environment scope.
 * @param name Nama variabel.
 * @return Pointer ke RuntimeBinding, atau NULL jika tidak ditemukan.
 */
RuntimeBinding *semFind(RuntimeEnv *env, const char *name);

/* ====================== Async Event Loop ==================== */

struct EventLoop *eventLoopCreate(void);
void eventLoopPush(struct EventLoop *loop, int handleId);
void eventLoopRun(Node *node, struct EventLoop *loop, RuntimeEnv *env,
                  Error *error);
bool eventLoopGetResult(struct EventLoop *loop, int handleId,
                        RuntimeValue *out);
void eventLoopDestroy(struct EventLoop *loop);

#endif
