#include <rupa.h>

#include "rewrite_internal.h"

/* ============================================================
 * rewrite.c — AST -> IR (entry point)
 *
 * Menurunkan AST Rupa menjadi IRModule berisi IRFunction/IRBlock
 * dan IRInstruction. IR sengaja independen dari RuntimeValue dan
 * NodeType (lihat lib/compiler/ir/ir.h): instruksi hanya memakai
 * IRValue/IRType, sehingga modul ini bisa dipakai sebagai dasar
 * transpile atau codegen tanpa menyentuh interpreter.
 *
 * Struktur hasil:
 *   - NODE_PROGRAM            -> IRFunction "main" + blok "entry"
 *   - NODE_FUNCTION_DECL      -> IRFunction tersendiri di module
 *   - Statement lain          -> instruksi di blok current
 *   - NODE_IF/NODE_LOOP/NODE_CASE -> blok + IR_BRANCH/IR_JUMP
 *
 * Control flow tidak menaruh flow di runtime: semua kontrol
 * berbentuk blok & branch sehingga urutan eksekusi ditentukan
 * penuh oleh IR. Konvensi nilai antar-blok bersifat phi-less:
 * temp yang belum di-store terbaca null (sequential store).
 *
 * Implementasi dipecah per tanggung jawab (rules.md: modular):
 *   rewrite_scope.c    — scope map (binding nama -> slot IR)
 *   rewrite_builder.c  — core builder: cache, type helper, fold
 *   rewrite_flow.c     — if/loop/case + break/continue
 *   rewrite_expr.c     — buildNode (ekspresi) + pipeSegments
 *   rewrite_stmt.c     — statement dispatcher + program/fungsi/assign
 * ============================================================ */

/* ==================== Entry point ==================== */

IRModule *rewrite(Node *node, int root, IRModule *ir) {
  if (!node || node->length <= 0 || !ir) return NULL;

  if (root < 0 || root >= node->length) {
    root = -1;
    for (int i = 0; i < node->length; i++) {
      if (node->ast[i].type == NODE_PROGRAM) {
        root = i;
        break;
      }
    }
    if (root < 0) return NULL;
  }

  IRBuilder b;
  builderInit(&b, node, ir);
  buildProgram(&b, node, root);
  return ir;
}
