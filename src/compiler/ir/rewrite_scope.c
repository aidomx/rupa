#include <rupa.h>

#include "rewrite_internal.h"

/* ============================================================
 * rewrite_scope.c — scope map IRBuilder (AST -> IR)
 *
 * Linked list binding nama variable -> IRValue slot. Scope baru
 * ditandai marker (name kosong + value = pointer builder sebagai
 * sentinel) sehingga scopePop berhenti di batas scope-nya.
 * ============================================================ */

IRValue *scopeFind(IRBuilder *b, const char *name) {
  if (!name) return NULL;
  for (ScopeMap *s = b->scopes; s; s = s->next)
    if (s->name && strcmp(s->name, name) == 0) return s->value;
  return NULL;
}

/* Type deklarasi variable (x: T = ...) untuk kontrak reassignment. */
const char *scopeTypeOf(IRBuilder *b, const char *name) {
  if (!name) return NULL;
  for (ScopeMap *s = b->scopes; s; s = s->next)
    if (s->name && strcmp(s->name, name) == 0) return s->type;
  return NULL;
}

void scopeBind(IRBuilder *b, const char *name, IRValue *value) {
  if (!name || !value) return;

  /* Rebind di scope sama: pertahankan type deklarasi lama (kontrak
   * reassignment tetap berlaku); type baru diset via scopeSetType. */
  for (ScopeMap *s = b->scopes; s; s = s->next) {
    if (s->name && strcmp(s->name, name) == 0) {
      s->value = value;
      return;
    }
  }

  ScopeMap *s = gccalloc(1, sizeof(*s));
  if (!s) return;
  s->name = gcdup(name);
  s->value = value;
  s->next = b->scopes;
  b->scopes = s;
}

/* Simpan/perbarui type deklarasi variable di scope map. */
void scopeSetType(IRBuilder *b, const char *name, const char *type) {
  if (!name || !type) return;
  for (ScopeMap *s = b->scopes; s; s = s->next) {
    if (s->name && strcmp(s->name, name) == 0) {
      s->type = gcdup(type);
      return;
    }
  }
}

void scopePush(IRBuilder *b) {
  ScopeMap *marker = gccalloc(1, sizeof(*marker));
  if (!marker) return;
  marker->name = gcdup("");
  marker->value = (IRValue *)b; /* sentinel: penanda batas scope */
  marker->next = b->scopes;
  b->scopes = marker;
}

void scopePop(IRBuilder *b) {
  while (b->scopes) {
    ScopeMap *top = b->scopes;
    int isMarker = top->name && top->name[0] == '\0' && (void *)top->value == (void *)b;

    b->scopes = top->next;
    if (isMarker) return;
  }
}
