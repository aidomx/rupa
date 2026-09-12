#include <rupa.h>

int grammarParseComment(Request *r, int a, int b, int *pos) {
  if (!r || a < 0 || b < 0) return GRAMMAR_NO_MATCH;

  Token *t = r->tokens;
  int id = -1;

  for (int i = a; i < b; i++) {
    int currType = t->data[i].type;

    if (currType == HASHTAG && i + 1 < b && t->data[i + 1].type == COMMENT) {
      /* # comment → NODE_INLINE_COMMENT */
      id = createComment(r->node, t->data[i + 1].value, NODE_INLINE_COMMENT);
    } else if (currType == SLASH && i + 1 < b && t->data[i + 1].type == COMMENT) {
      /* // or slash-star comment — distinguish by comment text value */
      const char *text = t->data[i + 1].value;
      if (text && text[0] == '/' && text[1] == '*') {
        /* slash-star ... star-slash → NODE_BLOCK_COMMENT */
        id = createComment(r->node, t->data[i + 1].value, NODE_BLOCK_COMMENT);
      } else {
        /* // comment → NODE_INLINE_COMMENT */
        id = createComment(r->node, t->data[i + 1].value, NODE_INLINE_COMMENT);
      }
    }
  }

  if (id < 0) return GRAMMAR_NO_MATCH;
  *pos = b;

  return id;
}
