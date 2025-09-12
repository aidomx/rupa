#include <rupa/package.h>

/**
 * @brief Menambahkan deklarasi ke dalam program node AST.
 *
 * @param node      Pointer ke struktur Node (AST global).
 * @param programId ID dari node program di dalam AST.
 * @param declId    ID dari node deklarasi yang akan ditambahkan.
 *
 * @details
 * - Mengecek validitas parameter (node, programId, declId).
 * - Memastikan target adalah node dengan tipe NODE_PROGRAM.
 * - Mengalokasikan AstDeclaration baru dan menambahkannya
 *   ke daftar deklarasi di dalam program.
 *
 * @note Jika terjadi error (misalnya invalid ID atau gagal malloc),
 *       fungsi akan menampilkan pesan error ke stderr dan keluar tanpa
 * perubahan.
 */
void addToProgram(Node *node, int programId, int declId) {
  if (!node || programId < 0 || programId >= node->length || declId < 0 ||
      declId >= node->length) {
    return;
  }

  if (node->ast[programId].type != NODE_PROGRAM) {
    fprintf(stderr, "Error: Node %d is not a program node\n", programId);
    return;
  }

  AstDeclaration *new_decl = malloc(sizeof(AstDeclaration));
  if (!new_decl) {
    perror("Failed to allocate declaration node");
    return;
  }

  new_decl->nodeId = declId;
  new_decl->next = NULL;

  AstDeclaration **current = &node->ast[programId].program.declarations;
  while (*current != NULL) {
    current = &(*current)->next;
  }
  *current = new_decl;
}

/**
 * @brief Membuat struktur Request untuk memproses token menjadi AST.
 *
 * @param tokens   Pointer ke daftar token yang akan diparse.
 * @param capacity Kapasitas awal untuk alokasi node AST.
 *
 * @return Struktur Request yang sudah terisi:
 * - tokens  → daftar token yang akan diproses
 * - node    → AST root yang baru dibuat
 * - program → ID program utama hasil createProgram()
 *
 * @note Struktur ini dipakai sebagai "state container" utama parser.
 */
Request createRequest(Token *tokens, int capacity) {
  Request req = {.tokens = tokens,
                 .node = createNode(capacity),
                 .left = -1,
                 .right.start = -1,
                 .right.end = -1,
                 .programId = createProgram(req.node)};

  return req;
}

/**
 * @brief Entry point parser untuk mengubah token list menjadi AST.
 *
 * @param tokens Pointer ke daftar token hasil tokenizer.
 *
 * @details
 * - Mengecek apakah token kosong → jika kosong, print "--- EMPTY ---".
 * - Membuat Request via createRequest().
 * - Memanggil processGenerate() untuk membangun AST.
 * - Menampilkan jumlah node hasil parsing.
 * - Memanggil startDebug() (opsional) untuk debugging AST.
 * - Membersihkan memori AST dengan clearNode().
 *
 * @note Ini adalah fungsi tingkat atas (top-level) untuk parsing.
 */
void generateAst(Token *tokens) {
  if (!tokens || tokens->length == 0) {
    printf("--- EMPTY ---\n");
    return;
  }

  printf("Total token: %d\n", tokens->length);

  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);

  if (node->length > 0) {
    printf("Total node: %d\n", node->length);
    // src/ast/debug.c
    startDebug(node); // debug print AST
  }

  clearNode(node);
}
