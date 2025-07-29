#include "ctypes.h"
#include "enum.h"
#include "limit.h"
#include "package.h"
#include "structure.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mengatur penugasan token berdasarkan identifier
void setTokenAssign();

// Mengatur token boolean
void setTokenBoolean();

// Mengatur token float
void setTokenFloat();

// Mengatur token identifier
void setTokenId();

// Mengatur token number
void setTokenNumber();

// Mengatur expresi token
void setTokenExpression();

// Mendapatkan token terakhir
void getCurrentToken();

// Mendapatkan lebih bayak token jika belum mencapai
// jumlah token yang tersimpan.
bool getMoreToken();

// Lanjut ke token berikutnya
void nextToken();

// Evaluasi parse token
void evalute();

//  Memparser hasil token menjadi AST
void parse(Token *token) {
  Parser parser = {.tokens = token, .length = 0};

  if (!parser.tokens) {
    return;
  }
}
