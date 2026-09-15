#pragma once
// Dari AstNode ke IR
#if defined(RUPA_PACKAGE_H)

/**
 * Menurunkan AST menjadi IRModule.
 *
 * @param node Pool AST sumber.
 * @param root Id node NODE_PROGRAM; <0 berarti dicari otomatis.
 * @param ir   Modul IR tujuan (dari createIR()).
 * @return Modul IR yang sama, atau NULL bila gagal.
 */
IRModule *rewrite(Node *n, int root, IRModule *ir);

#endif
