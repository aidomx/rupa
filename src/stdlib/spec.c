#include <rupa.h>

/**
 * @brief Modul `spec` — akses konfigurasi .spec dari user code.
 *
 *   import spec from rupa
 *   net.connect(spec.settings.host, spec.settings.port)
 *
 * Provenance (keputusan user):
 *   - print(spec)                -> SELALU ditolak (SpecError)
 *   - specx = spec; print(specx) -> ditolak (tag menempel di value)
 *   - v = "127.0.0.1"; print(v)  -> bebas (variable biasa)
 *   - print(spec.settings.host)  -> OK (leaf scalar)
 *   - print(spec.settings)       -> ditolak (sub-object bertag)
 *
 * Tag `__spec` disisipkan SAAT KONSTRUKSI — RuntimeValue object disalin
 * by value, jadi tag ikut ke alias; value tanpa tag tetap bebas.
 */

/* ==================== holder (diisi rupa go) ==================== */

static RuntimeValue g_specModule = {0};
static bool g_specLoaded = false;

void specModuleProvide(RuntimeValue module) {
  g_specModule = module;
  g_specLoaded = true;
}

void specModuleClear(void) {
  g_specModule = valueNull();
  g_specLoaded = false;
}

/* ==================== konstruktor bertag __spec ==================== */

void specTagObject(RuntimeValue *obj) {
  if (!obj || obj->type != VALUE_OBJECT) return;
  valueObjectSet(obj, "__spec", valueBoolean(true));
}

/* Port dibawa sebagai NUMBER — pemakaian natural:
 * net.connect(spec.settings.host, spec.settings.port). */
static RuntimeValue specPortValue(const char *s) {
  long long n = s && *s ? atoll(s) : 0;
  if (n <= 0 || n > 65535) n = 0;
  return valueNumber(n);
}

/* Helper rupa go: bangun object settings bertag dari GoSpec. */
RuntimeValue specModuleBuild(const char *host, const char *port, const char *protocol,
                             const char *dbhost, const char *dbport, const char *dbname,
                             const char *dbuser, const char *domain) {
  RuntimeValue settings = valueObject(NULL);
  valueObjectSet(&settings, "host", valueString(host ? host : "-"));
  valueObjectSet(&settings, "port", specPortValue(port));
  valueObjectSet(&settings, "protocol", valueString(protocol ? protocol : "http"));
  valueObjectSet(&settings, "dbhost", valueString(dbhost ? dbhost : "-"));
  valueObjectSet(&settings, "dbport", specPortValue(dbport));
  valueObjectSet(&settings, "dbname", valueString(dbname ? dbname : "-"));
  valueObjectSet(&settings, "dbuser", valueString(dbuser ? dbuser : "-"));
  specTagObject(&settings);

  RuntimeValue mod = valueObject(NULL);
  valueObjectSet(&mod, "settings", settings);
  valueObjectSet(&mod, "domain", valueString(domain ? domain : "-"));
  valueObjectSet(&mod, "target", valueString("-"));
  specTagObject(&mod);
  return mod;
}

/* ==================== init module ==================== */

InterpreterResult stdSpecInit(Node *node, int id, RuntimeEnv *env, Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  if (!g_specLoaded) {
    /* Di luar `rupa go` (script biasa): modul ada tapi kosong — akses
     * field menghasilkan null. Pesan jelas di print guard bila user
     * mencoba print object hasil .spec. */
    return resultNormal(valueObject(NULL));
  }
  return resultNormal(g_specModule);
}
