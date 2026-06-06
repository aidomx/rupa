#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Inisialisasi atom awal dari input.
 *
 * Atom merepresentasikan unit terkecil yang dapat di-lex
 * (identifier, number, underscore, dll).
 *
 * @param input Pointer ke Input
 * @return Atom terinisialisasi atau zeroed Atom jika gagal
 */
extern Atom init_atom(Input *input);

/**
 * @brief Menentukan atom utama (primary atom) pada posisi cursor.
 *
 * Fungsi ini hanya bertugas mengaktifkan flags->isIdentifier
 * tanpa memindahkan alur eksekusi lexer.
 *
 * @param atom Pointer ke Atom
 * @param content Source input
 * @param flags Lexer flags
 */
void primaryAtom(Atom *atom, const char *content, Flags *flags);

extern int scanAtom(Atom *atom);

#endif
