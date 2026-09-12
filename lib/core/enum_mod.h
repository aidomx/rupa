#pragma once
/**
 * Tipe untuk design module baru (AstMod) — 1 container untuk
 * 2 job: import dan export.
 *
 * @ModEntryKind
 * @ModType
 */

/**
 * Bentuk entry module.
 */
enum ModEntryKind {
  MOD_ID     = 0, /* x                            */
  MOD_MEMBER = 1, /* x.y — sub-path di childrens  */
  MOD_WILD   = 2, /* x.*                          */
};

/**
 * Jenis statement module.
 */
enum ModType {
  ImportDecl = 0,
  ExportDecl = 1,
};
