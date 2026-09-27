#include <rupa.h>

/* ============================================================
 * print_format.c — format engine print (design/next_print.txt)
 *
 * Pola 3 — printf-style: print("%s\n", "hello")
 * Pola 4 — stream target:   print(stderr, "err\n")
 *
 * Modul ini sengaja terpisah dari value.c (render nilai + pola 1)
 * dan statement.c (eksekusi NODE_PRINT, pola 2) supaya tiap file
 * tetap kecil dan tugasnya tunggal (rules.md: modular).
 *
 * Satu pintu output: printRenderArgs() merender seluruh args ke
 * satu buffer lalu menulis ke stream tujuan — dipakai interpreter
 * (statement.c), mesin IR (execute.c), dan REPL.
 * ============================================================ */

/* ==================== Pola 4: stream target ==================== */

/* Binding `stdout` / `stderr` (registered di builtins.c) membawa
 * VALUE_PTR ke FILE*. VALUE_PTR juga dipakai handle memori (pin/
 * Contract) — jadi hanya pointer identik dengan stream C asli yang
 * diterima; handle memori tidak mungkin lolos, tanpa fileno
 * (portable, tanpa asumsi layout FILE). */
FILE *printStreamOf(RuntimeValue v) {
  if (v.type != VALUE_PTR || !v.as.ptr) return NULL;
  if (v.as.ptr == (void *)stdout) return stdout;
  if (v.as.ptr == (void *)stderr) return stderr;
  return NULL;
}

/* ==================== Pola 3: printf-style ==================== */

static bool isSpecChar(char c) {
  switch (c) {
  case 's': case 'd': case 'i': case 'u': case 'x': case 'X': case 'o':
  case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'c':
  case '%':
    return true;
  default:
    return false;
  }
}

/* Hitung konversi valid %[flags][width][.prec]spec (%% tidak dihitung).
 * >0 berarti string layak dipakai sebagai format (pola 3); string
 * polos seperti "50%" atau "a%b" menghasilkan 0 — dicetak apa adanya. */
static int countSpecs(const char *s) {
  int n = 0;
  for (const char *p = s; *p; p++) {
    if (*p != '%') continue;
    const char *q = p + 1;
    while (*q == '-' || *q == '+' || *q == '0' || (*q >= '0' && *q <= '9') || *q == '.')
      q++;
    if (!*q) break;
    if (isSpecChar(*q) && *q != '%') n++;
    p = q;
  }
  return n;
}

/* Konsumsi satu konversi %[flags][width][.prec]spec pada fmt dan
 * render hasilnya. Return jumlah karakter yang dikonsumsi (0 bila
 * bukan konversi valid — '%' dicetak literal oleh caller). */
static int formatOneSpec(char **out, size_t *len, size_t *cap, const char *fmt,
                         RuntimeValue arg, Error *error, bool *argUsed) {
  const char *q = fmt + 1;
  while (*q == '-' || *q == '+' || *q == '0' || (*q >= '0' && *q <= '9') || *q == '.')
    q++;
  if (!*q || !isSpecChar(*q)) return 0;
  char spec = *q;
  int consumed = (int)(q - fmt) + 1;

  if (spec == '%') {
    printBufAppend(out, len, cap, "%");
    return consumed;
  }

  /* %s — semua tipe di-render sejajar pola 2 (meta "__*" disembunyikan,
   * print(spec) ditolak). String dicetak mentah: format dan interpolasi
   * adalah pola terpisah (design/next_print.txt). */
  if (spec == 's') {
    if (valueSpecRejected(arg, error)) return consumed;
    valuePrintTo(out, len, cap, arg);
    *argUsed = true;
    return consumed;
  }

  /* Konversi C dari body rupa: '%' + body + spec (semua spec yang
   * dikenali rupa valid juga sebagai konversi C untuk kind terkait). */
  char conv[64];
  if ((size_t)(q - fmt) + 2 > sizeof(conv)) return 0;
  memcpy(conv, fmt, (size_t)(q - fmt));
  conv[q - fmt] = spec;
  conv[q - fmt + 1] = '\0';

  enum { K_CHR, K_DBL, K_INT, K_UINT } kind;
  char ch = 0;
  double d = 0;
  long long ll = 0;
  unsigned long long ull = 0;

  switch (spec) {
  case 'c':
    kind = K_CHR;
    if (arg.type == VALUE_NUMBER)
      ch = (char)arg.as.number;
    else if (arg.type == VALUE_STRING && arg.as.string && arg.as.string[0])
      ch = arg.as.string[0];
    break;
  case 'f': case 'F': case 'e': case 'E': case 'g': case 'G':
    kind = K_DBL;
    d = arg.type == VALUE_DECIMAL  ? arg.as.decimal
        : arg.type == VALUE_NUMBER ? (double)arg.as.number
                                   : 0.0;
    break;
  case 'd': case 'i':
    kind = K_INT;
    ll = arg.type == VALUE_NUMBER    ? arg.as.number
         : arg.type == VALUE_DECIMAL ? (long long)arg.as.decimal
         : arg.type == VALUE_BOOLEAN ? (arg.as.boolean ? 1 : 0)
                                     : 0;
    break;
  default: /* u x X o */
    kind = K_UINT;
    ull = arg.type == VALUE_NUMBER ? (unsigned long long)arg.as.number : 0;
    break;
  }

  char stackbuf[256];
  int n;
  switch (kind) {
  case K_CHR: n = snprintf(stackbuf, sizeof(stackbuf), conv, ch); break;
  case K_DBL: n = snprintf(stackbuf, sizeof(stackbuf), conv, d); break;
  case K_INT: n = snprintf(stackbuf, sizeof(stackbuf), conv, ll); break;
  default: n = snprintf(stackbuf, sizeof(stackbuf), conv, ull); break;
  }
  if (n < 0) return 0;
  if ((size_t)n < sizeof(stackbuf)) {
    printBufAppend(out, len, cap, stackbuf);
  } else {
    /* Width besar / %f presisi tinggi: format ulang ke heap. */
    char *big = malloc((size_t)n + 1);
    if (!big) return 0;
    switch (kind) {
    case K_CHR: snprintf(big, (size_t)n + 1, conv, ch); break;
    case K_DBL: snprintf(big, (size_t)n + 1, conv, d); break;
    case K_INT: snprintf(big, (size_t)n + 1, conv, ll); break;
    default: snprintf(big, (size_t)n + 1, conv, ull); break;
    }
    printBufAppend(out, len, cap, big);
    free(big);
  }
  *argUsed = true;
  return consumed;
}

/* Render string format + args ke buffer (pola 3). Tanpa konversi
 * valid, semua karakter dicetak apa adanya — string biasa aman. */
static void formatPrintString(char **out, size_t *len, size_t *cap, const char *fmt,
                              RuntimeValue *args, int argc, Error *error) {
  const char *p = fmt;
  int next = 0;
  while (*p) {
    if (*p != '%') {
      char c[2] = {*p, 0};
      printBufAppend(out, len, cap, c);
      p++;
      continue;
    }
    bool used = false;
    int consumed =
        formatOneSpec(out, len, cap, p, next < argc ? args[next] : valueNull(),
                      error, &used);
    if (consumed > 0) {
      if (used) next++;
      p += consumed;
    } else {
      /* '%' + spec tak dikenal: cetak '%' apa adanya, lanjut char. */
      printBufAppend(out, len, cap, "%");
      p++;
    }
  }
}

/* ==================== Color print ==================== */

/* Bungkus rentang args yang dirender dengan escape ANSI truecolor
 * (design/next_print.txt Color print): arg color (U_RED bawaan atau
 * enum custom bertipe color) menyusul stream opsional, reset "\x1b[0m"
 * ditulis di akhir. valueColorOf() sejajar printStreamOf() — keduanya
 * mengintip args[0..] tanpa mengubah kontrak argc/argv print(). */
static void printBufAppendColor(char **out, size_t *len, size_t *cap, long long rgb) {
  char seq[32];
  int r = (int)((rgb >> 16) & 0xFF);
  int g = (int)((rgb >> 8) & 0xFF);
  int b = (int)(rgb & 0xFF);
  snprintf(seq, sizeof(seq), "\x1b[38;2;%d;%d;%dm", r, g, b);
  printBufAppend(out, len, cap, seq);
}

/* ==================== Render args print ==================== */

/* Pola 1 & 2 per arg: string lewat interpolasi (bila interp), tipe
 * lain lewat valuePrintTo. print(spec) ditolak sejajar interpreter. */
static void renderOneArg(char **out, size_t *len, size_t *cap, RuntimeValue v,
                         RuntimeEnv *env, Error *error, bool interp) {
  if (valueSpecRejected(v, error)) return;
  if (v.type == VALUE_STRING) {
    if (interp)
      printStringInterpTo(out, len, cap, v.as.string, env, error);
    else
      printBufAppend(out, len, cap, v.as.string ? v.as.string : "");
    return;
  }
  valuePrintTo(out, len, cap, v);
}

int printRenderArgs(RuntimeValue *args, int argc, RuntimeEnv *env, Error *error,
                    FILE *fallback, bool interp, bool *streamed) {
  *streamed = false;
  if (argc <= 0 || !args) return 0;

  /* Pola 4: arg pertama stdout/stderr = stream target (design: "karena
   * print sudah bisa membaca banyak args, jadi jika ketemu stderr atau
   * stdout ini sudah masuk jalur stdin"). */
  FILE *out = printStreamOf(args[0]);
  int first = out ? 1 : 0;
  if (!out) out = fallback;
  if (first >= argc) return first; /* print(stderr) tanpa sisa arg */

  /* Color print: arg berikutnya (setelah stream opsional) boleh berupa
   * color (U_RED bawaan / enum custom bertipe color) — sisanya
   * dirender lalu dibungkus ANSI truecolor + reset. */
  long long rgb = 0;
  bool isColor = valueColorOf(args[first], &rgb);
  if (isColor) first++;
  if (first >= argc) return first; /* print(U_RED) tanpa sisa arg */

  char *buf = NULL;
  size_t len = 0, cap = 0;
  if (isColor) printBufAppendColor(&buf, &len, &cap, rgb);

  /* Pola 3: arg pertama (setelah stream) string ber-format + ada sisa
   * arg. String polos tidak terpengaruh (countSpecs == 0). */
  const char *fmt = NULL;
  int specs = 0;
  if (args[first].type == VALUE_STRING && args[first].as.string && first + 1 < argc) {
    specs = countSpecs(args[first].as.string);
    if (specs > 0) fmt = args[first].as.string;
  }

  if (fmt) {
    formatPrintString(&buf, &len, &cap, fmt, args + first + 1, argc - first - 1, error);
    /* Arg lebih banyak dari konversi: sambung dengan spasi (pola 2). */
    int used = argc - first - 1 < specs ? argc - first - 1 : specs;
    for (int i = first + 1 + used; i < argc; i++) {
      printBufAppend(&buf, &len, &cap, " ");
      renderOneArg(&buf, &len, &cap, args[i], env, error, interp);
    }
  } else {
    for (int i = first; i < argc; i++) {
      if (i > first) printBufAppend(&buf, &len, &cap, " ");
      renderOneArg(&buf, &len, &cap, args[i], env, error, interp);
    }
  }

  if (isColor) printBufAppend(&buf, &len, &cap, "\x1b[0m");

  if (buf) {
    fwrite(buf, 1, len, out);
    free(buf);
  }
  if (out == stderr) fflush(stderr);
  *streamed = (out == stderr); /* stdout ikut buffering standar */
  return first;
}
