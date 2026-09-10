#include <rupa.h>

// Fungsi pemanggil awal
void startDebug(Node *node) {
  if (!node || node->length == 0) return;

  printf("--- Struktur AST Node ---\n");

  // ✅ Cari program node (biasanya node pertama)
  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_PROGRAM) {
      printAst(node, i, 0);
      printf("--- ENDOF ---\n");
      return;
    }
  }

  // Fallback: jika tidak ada program node, print semua root nodes
  for (int i = 0; i < node->length; i++) {
    bool isRoot = true;
    for (int j = 0; j < node->length; j++) {
      AstNode *n = &node->ast[j];
      if ((n->type == NODE_ASSIGN && (n->assign.target == i || n->assign.value == i)) ||
          (n->type == NODE_BINARY && (n->binary.left == i || n->binary.right == i)) ||
          (n->type == NODE_SUBSCRIPT && (n->subscript.posId == i || n->subscript.index == i)) ||
          (n->type == NODE_RETURN && n->asReturn.expression == i)) {
        isRoot = false;
        break;
      }
    }
    if (isRoot) {
      printAst(node, i, 0);
    }
  }

  printf("--- ENDOF ---\n");
}
