#pragma once
#define RUPA_PACKAGE_H

/*
 * Header publik rupa — SATU titik include untuk konsumer librupa:
 *
 *     #include <rupa.h>
 *
 * Link:  cc -std=gnu11 -Iinclude -I. main.c lib/librupa.a -lm -lpthread \
 *            -lssl -lcrypto -o app
 * atau   cc ... main.c lib/librupa.so ... (shared)
 *
 * Entry point resmi runtime = bootstrap loader():
 *     int main(int argc, char *argv[]) { return loader(argv, argc); }
 *
 * API utama yang terekspos:
 *   - loader(argv, argc)      — CLI/bootstrap (src/bootstrap/loader.c)
 *   - execute(code)           — evaluasi kode rupa dari C
 *   - semCreateEnv/semSet/semGet/semMarkPub/semHasPub — scope & binding
 *   - value* (valueNumber, valueString, valueObjectGet, ...)
 *   - createIR/irModuleFree/debugIRModule — IR + disassembler
 *   - executeIR/compileIR     — eksekusi & backend C
 *   - gcinit/gcclean/gcstrdup — arena GC (wajib gcinit sebelum pakai)
 *
 * Pemisahan ini menjaga API publik tetap ringkas dan stabil, sedangkan
 * utilitas serta implementasi internal dapat diubah tanpa memengaruhi
 * antarmuka utama.
 */
#include "../lib/intl.h"
