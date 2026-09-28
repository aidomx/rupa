#include <rupa.h>
/* setelah rupa.h: guard-nya bergantung RUPA_PACKAGE_H */
#include "module.h"

/* loader_path.c — utilitas file & resolusi path import: relatif/dotted,
 * absolut, dan package boundary. Unit hasil split loader.c:
 *   loader_state.c   — state global + module cache
 *   loader_path.c    — resolusi path (file ini)
 *   loader_export.c  — pembangunan object export entries
 *   loader.c         — entry: loadModuleFile/loadModuleFileError
 */

/* ---- Internal utility functions ---- */

bool modFileExists(const char *path) {
  if (!path) return false;
  FILE *f = fopen(path, "rb");
  if (f) {
    fclose(f);
    return true;
  }
  return false;
}

char *modResolveModulePath(const char *module_path) {
  if (!module_path) return NULL;
  const char *path = module_path;
  if (path[0] == '.' && path[1] == '/') path += 2;
  int len = (int)strlen(path);
  char *buf = malloc(len + 4);
  if (!buf) return NULL;
  for (int i = 0; i < len; i++)
    buf[i] = path[i] == '.' ? '/' : path[i];
  buf[len] = '\0';
  strcat(buf, ".rp");
  return buf;
}

char *modJoinPath(const char *dir, const char *rel) {
  if (!dir || !rel) return rel ? strdup(rel) : NULL;
  int dlen = (int)strlen(dir);
  int rlen = (int)strlen(rel);
  int need_sep = (dlen > 0 && dir[dlen - 1] != '/');
  char *buf = malloc(dlen + need_sep + rlen + 1);
  if (!buf) return NULL;
  memcpy(buf, dir, dlen);
  if (need_sep) buf[dlen] = '/';
  memcpy(buf + dlen + need_sep, rel, rlen + 1);
  return buf;
}

char *modDirName(const char *path) {
  if (!path) return NULL;
  const char *last_slash = strrchr(path, '/');
  if (!last_slash) return strdup(".");
  int len = (int)(last_slash - path);
  if (len == 0) return strdup("/");
  char *buf = malloc(len + 1);
  if (!buf) return NULL;
  memcpy(buf, path, len);
  buf[len] = '\0';
  return buf;
}

bool hasDotSlash(const char *path) {
  if (!path || path[0] != '.') return false;
  if (path[1] == '/') return true;
  if (path[1] == '.' && path[2] == '/') return true;
  return false;
}

/* Path kanonik untuk identitas modul: realpath menyelesaikan symlink
 * dan '..' sehingga "./rpx" dan "rpx" dari source dir yang sama
 * menghasilkan string identik. File yang belum ada fallback ke path
 * apa adanya (readfile akan gagal nanti seperti biasa). */
char *moduleCanonicalPath(const char *path) {
  if (!path) return NULL;
  char *resolved = RUPA_REALPATH(path);
  if (!resolved) return gcstrdup(path); /* best effort */
  char *out = gcstrdup(resolved);
  free(resolved);
  return out;
}

/* Apakah file punya statement export eksplisit? Scan tekstual ringan —
 * gate resolusi ie.txt import #5: Z-self hanya dihitung bila Z mengekspor
 * dirinya ("leaf polos tanpa export tidak dihitung → harus via Y").
 * `export *` dan `namespace` sama-sama dihitung (setara has_export di
 * loader); baris komentar // dan # di-skip. */
static bool fileHasExportStatement(const char *path) {
  if (!path) return false;
  FILE *f = fopen(path, "r");
  if (!f) return false;
  char line[512];
  bool found = false;
  while (!found && fgets(line, sizeof(line), f)) {
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (p[0] == '/' && p[1] == '/') continue;
    if (p[0] == '#') continue;
    bool is_export = strncmp(p, "export", 6) == 0 &&
                     (p[6] == '\0' || p[6] == ' ' || p[6] == '\t' ||
                      p[6] == '*' || p[6] == '\n' || p[6] == '\r');
    bool is_namespace = strncmp(p, "namespace", 9) == 0 &&
                        (p[9] == '\0' || p[9] == ' ' || p[9] == '\t');
    if (is_export || is_namespace) found = true;
  }
  fclose(f);
  return found;
}

char *resolveDotPath(const char *module_path, const char *source_dir) {
  if (!module_path || !source_dir) return NULL;

  /* Dual resolution (design/ie.txt import #5): dotted path dicoba sebagai
   * file/module nyata DULU — Z yang mengekspor dirinya sendiri membuat
   * X.Y hanya referensi path — baru fallback ke parent index (Z menjadi
   * sub-module via export Y). Keduanya tidak ada → NULL → ImportError. */
  const char *p = module_path;
  while (p[0] == '.' && p[1] == '.' && p[2] == '/') p += 3;
  if (p[0] == '.' && p[1] == '/') p += 2;
  const char *lastDot = strrchr(p, '.');
  if (lastDot && lastDot[1] != '\0' && lastDot[1] != '/') {
    size_t plen = (size_t)(lastDot - module_path); /* prefix + segmen parent */
    char *dirpart = malloc(plen + 1);
    if (!dirpart) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < plen; i++) {
      char c = module_path[i];
      if (c == '.' && i + 1 < plen && module_path[i + 1] == '.') {
        dirpart[j++] = '.';
        dirpart[j++] = '.';
        i++;
      } else if (c == '.') {
        dirpart[j++] = '/';
      } else {
        dirpart[j++] = c;
      }
    }
    while (j > 0 && dirpart[j - 1] == '/') j--; /* buang '/' ekor sebelum gabung */
    dirpart[j] = '\0';
    /* Nama leaf = segmen terakhir setelah dot terakhir (Z pada X.Y.Z). */
    const char *leaf = lastDot + 1;
    size_t llen = strlen(leaf);

    /* 1) Z-self sebagai file: ./X.Y/Z.rp — leaf mengekspor dirinya. */
    char *leaf_rel = malloc(j + 1 + llen + 4);
    if (leaf_rel) {
      memcpy(leaf_rel, dirpart, j);
      leaf_rel[j] = '/';
      memcpy(leaf_rel + j + 1, leaf, llen);
      strcpy(leaf_rel + j + 1 + llen, ".rp");
      char *full = modJoinPath(source_dir, leaf_rel);
      free(leaf_rel);
      /* ie.txt #5: file ada TAPI tanpa statement export → bukan Z-self,
       * lanjut ke tree navigation (parent index). */
      if (full && modFileExists(full) && fileHasExportStatement(full)) {
        free(dirpart);
        return full;
      }
      free(full);
    }

    /* 2) Z-self sebagai folder: ./X.Y/Z/index.rp. */
    char *leafdir_rel = malloc(j + 1 + llen + 10);
    if (leafdir_rel) {
      memcpy(leafdir_rel, dirpart, j);
      leafdir_rel[j] = '/';
      memcpy(leafdir_rel + j + 1, leaf, llen);
      strcpy(leafdir_rel + j + 1 + llen, "/index.rp");
      char *full = modJoinPath(source_dir, leafdir_rel);
      free(leafdir_rel);
      if (full && modFileExists(full) && fileHasExportStatement(full)) {
        free(dirpart);
        return full;
      }
      free(full);
    }

    /* 3) Tree navigation: ./X.Y/index.rp — Z sub-module via export parent. */
    char *idx_rel = malloc(j + 10);
    if (!idx_rel) {
      free(dirpart);
      return NULL;
    }
    memcpy(idx_rel, dirpart, j);
    idx_rel[j] = '\0';
    strcat(idx_rel, "/index.rp");
    char *full = modJoinPath(source_dir, idx_rel);
    free(dirpart);
    free(idx_rel);
    if (full && modFileExists(full)) return full;
    free(full);
    return NULL;
  }

  int mlen = (int)strlen(module_path);
  char *rel = malloc(mlen + 16);
  if (!rel) return NULL;

  int j = 0;
  for (int i = 0; i < mlen; i++) {
    if (module_path[i] == '.' && i + 1 < mlen && module_path[i + 1] == '.') {
      rel[j++] = '.';
      rel[j++] = '.';
      i++;
    } else if (module_path[i] == '.') {
      rel[j++] = '/';
    } else {
      rel[j++] = module_path[i];
    }
  }
  rel[j] = '\0';

  char *file_rel = malloc(j + 4);
  if (file_rel) {
    memcpy(file_rel, rel, j);
    file_rel[j] = '\0';
    strcat(file_rel, ".rp");
    char *full = modJoinPath(source_dir, file_rel);
    free(file_rel);
    if (full && modFileExists(full)) {
      free(rel);
      return full;
    }
    free(full);
  }

  char *dir_rel = malloc(j + 14);
  if (dir_rel) {
    memcpy(dir_rel, rel, j);
    dir_rel[j] = '\0';
    strcat(dir_rel, "/index.rp");
    char *full = modJoinPath(source_dir, dir_rel);
    free(dir_rel);
    if (full && modFileExists(full)) {
      free(rel);
      return full;
    }
    free(full);
  }

  free(rel);
  return NULL;
}

/* ---- Import absolut ----
 * "/abs/path" atau "/abs/X.Y.Z". Dicoba berurutan:
 *   1) file eksplisit (stat reguler — mencakup /x/lib.rp),
 *   2) direktori → dir/index.rp (package),
 *   3) bukan berakhiran .rp: + ".rp", lalu + "/index.rp",
 *   4) bentuk dotted ("/tmp/rpx_engine.view") → resolveDotPath
 *      terhadap "/" (tiap segmen = direktori).
 * Return path malloc'd; NULL bila tidak ada yang cocok. */
static bool absIsFile(const char *p) {
  struct stat st;
  return p && stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static bool absIsDir(const char *p) {
  struct stat st;
  return p && stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool absEndsWith(const char *s, const char *suffix) {
  size_t sl = strlen(s), ul = strlen(suffix);
  return sl >= ul && strcmp(s + sl - ul, suffix) == 0;
}

char *resolveAbsModulePath(const char *module_path) {
  if (!module_path || module_path[0] != '/') return NULL;

  /* 1) File eksplisit — termasuk path .rp dari stdlib loader. */
  if (absIsFile(module_path)) return strdup(module_path);

  /* 2) Direktori → package index. */
  if (absIsDir(module_path))
    return modJoinPath(module_path, "index.rp");

  if (absEndsWith(module_path, ".rp")) return NULL;

  const char *lastDot = strrchr(module_path, '.');
  bool dotted = lastDot && lastDot[1] != '\0' && lastDot[1] != '/';

  /* 3) + ".rp" */
  size_t len = strlen(module_path);
  char *f = malloc(len + 4);
  if (!f) return NULL;
  memcpy(f, module_path, len);
  strcpy(f + len, ".rp");
  if (absIsFile(f)) return f;
  free(f);

  /* + "/index.rp" */
  char *idx = malloc(len + 10);
  if (!idx) return NULL;
  memcpy(idx, module_path, len);
  strcpy(idx + len, "/index.rp");
  if (absIsFile(idx)) return idx;
  free(idx);

  /* 4) Dotted: tiap segmen = direktori. */
  if (dotted) {
    char *source_dir = strdup("/");
    if (!source_dir) return NULL;
    char *full = resolveDotPath(module_path, source_dir);
    free(source_dir);
    return full;
  }

  return NULL;
}

/* ---- Package boundary ---- */

/* Dir berisi index.rp? */
static bool dirHasIndex(const char *dir) {
  if (!dir || !*dir) return false;
  char *p = modJoinPath(dir, "index.rp");
  if (!p) return false;
  bool ok = modFileExists(p);
  free(p);
  return ok;
}

/* Package root terluar yang memuat file: naik dari dir file, ingat
 * ancestor terjauh yang punya index.rp; berhenti di gap pertama
 * SETELAH package ditemukan. NULL bila file di luar package manapun. */
char *packageRootOf(const char *file_path) {
  if (!file_path || !*file_path) return NULL;
  char *outermost = NULL;
  char *dir = modDirName(file_path);
  for (int depth = 0; dir && depth < 32; depth++) {
    char *parent = modDirName(dir);
    bool atRoot = !parent || strcmp(parent, dir) == 0;
    if (dirHasIndex(dir)) {
      free(outermost);
      outermost = dir; /* kepemilikan pindah ke outermost */
    } else {
      free(dir);
      if (outermost) { /* gap setelah package — stop */
        free(parent);
        break;
      }
    }
    if (atRoot) {
      free(parent);
      break;
    }
    dir = parent;
  }
  return outermost;
}

/* Apakah dotted path ini resolve ke FILE leaf sungguhan — bukan index
 * parent dan bukan leaf di dalam package (yang akan di-redirect loader
 * ke parent-nya)? Dipakai dispatch untuk memilih semantik ie.txt:
 * leaf → single entry boleh otomatis namespace (#4); via index parent →
 * member wajib ada (tree nav #5). */
bool modSourceResolvesLeaf(const char *module_path) {
  if (!module_path || !hasDotSlash(module_path)) return false;
  char *source_dir = modDirName(g_source_file_path);
  if (!source_dir) return false;
  char *full = resolveDotPath(module_path, source_dir);
  free(source_dir);
  if (!full) return false;
  size_t len = strlen(full);
  bool is_leaf = !(len >= 8 && strcmp(full + len - 8, "/index.rp") == 0);
  if (is_leaf) {
    /* Leaf di dalam package: loader hanya me-redirect bila caller di
     * LUAR package. Caller se-package (mis. open.rp → ./dsn) = muat
     * langsung → semantik leaf. Caller luar → redirect parent index. */
    char *pkg_root = packageRootOf(full);
    if (pkg_root) {
      char *caller = moduleCanonicalPath(g_source_file_path);
      if (!caller) {
        is_leaf = false; /* tanpa konteks caller: anggap redirect */
      } else {
        size_t plen = strlen(pkg_root);
        bool inside = strncmp(caller, pkg_root, plen) == 0 &&
                      (caller[plen] == '/' || caller[plen] == '\0');
        is_leaf = inside;
        gcfree(caller);
      }
      free(pkg_root);
    }
  }
  free(full);
  return is_leaf;
}
