#include <rupa.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

/* ==================== Validation helpers ==================== */

static InterpreterResult cryptoError(Error *error, const char *name,
                                    const char *message) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)"CryptoError",
                                .message = (char *)message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  (void)name;
  return resultFlow(FLOW_ERROR, valueNull());
}

/* ==================== Helper: bytes to hex string ==================== */

static char *bytesToHex(const unsigned char *bytes, int len) {
  char *hex = gcmall(len * 2 + 1);
  for (int i = 0; i < len; i++)
    sprintf(hex + i * 2, "%02x", bytes[i]);
  hex[len * 2] = '\0';
  return hex;
}

/* ==================== Generic EVP hash ==================== */

static InterpreterResult evpHash(const char *algo, const char *input,
                                 int inputLen, Error *error) {
  const EVP_MD *md = NULL;
  if (strcmp(algo, "md5") == 0)
    md = EVP_md5();
  else if (strcmp(algo, "sha1") == 0)
    md = EVP_sha1();
  else if (strcmp(algo, "sha256") == 0)
    md = EVP_sha256();
  else if (strcmp(algo, "sha512") == 0)
    md = EVP_sha512();
  else
    return cryptoError(error, algo, "unknown algorithm");

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hashLen = 0;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return cryptoError(error, algo, "EVP_MD_CTX_new failed");

  if (EVP_DigestInit_ex(ctx, md, NULL) != 1 ||
      EVP_DigestUpdate(ctx, input, inputLen) != 1 ||
      EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
    EVP_MD_CTX_free(ctx);
    return cryptoError(error, algo, "digest failed");
  }
  EVP_MD_CTX_free(ctx);

  return resultNormal(valueString(bytesToHex(hash, hashLen)));
}

/* ==================== crypto.md5(str) ==================== */

static InterpreterResult cryptoMd5(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                   Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return cryptoError(error, "md5", "crypto.md5() expects a string");
  return evpHash("md5", argv[0].as.string, strlen(argv[0].as.string), error);
}

/* ==================== crypto.sha1(str) ==================== */

static InterpreterResult cryptoSha1(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                    Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return cryptoError(error, "sha1", "crypto.sha1() expects a string");
  return evpHash("sha1", argv[0].as.string, strlen(argv[0].as.string), error);
}

/* ==================== crypto.sha256(str) ==================== */

static InterpreterResult cryptoSha256(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return cryptoError(error, "sha256", "crypto.sha256() expects a string");
  return evpHash("sha256", argv[0].as.string, strlen(argv[0].as.string), error);
}

/* ==================== crypto.sha512(str) ==================== */

static InterpreterResult cryptoSha512(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return cryptoError(error, "sha512", "crypto.sha512() expects a string");
  return evpHash("sha512", argv[0].as.string, strlen(argv[0].as.string), error);
}

/* ==================== crypto.hmac(key, message) ==================== */

static InterpreterResult cryptoHmac(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                    Error *error) {
  (void)env;
  if (argc < 2)
    return cryptoError(error, "hmac", "crypto.hmac() expects key and message");
  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return cryptoError(error, "hmac", "key must be string");
  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return cryptoError(error, "hmac", "message must be string");

  const char *key = argv[0].as.string;
  int keyLen = strlen(key);
  const char *msg = argv[1].as.string;
  int msgLen = strlen(msg);

  unsigned char mac[EVP_MAX_MD_SIZE];
  unsigned int macLen = 0;

  unsigned char *result =
      HMAC(EVP_sha256(), key, keyLen, (unsigned char *)msg, msgLen, mac, &macLen);
  if (!result)
    return cryptoError(error, "hmac", "HMAC failed");

  return resultNormal(valueString(bytesToHex(mac, macLen)));
}

/* ==================== crypto.base64Encode(str) ==================== */

static InterpreterResult cryptoBase64Encode(int argc, RuntimeValue *argv,
                                            RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return cryptoError(error, "base64Encode",
                       "crypto.base64Encode() expects a string");

  const char *input = argv[0].as.string;
  int inputLen = strlen(input);
  int outputLen = 4 * ((inputLen + 2) / 3);
  char *output = gcmall(outputLen + 1);

  const char *chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i, j;
  for (i = 0, j = 0; i < inputLen;) {
    unsigned int a = i < inputLen ? (unsigned char)input[i++] : 0;
    unsigned int b = i < inputLen ? (unsigned char)input[i++] : 0;
    unsigned int c = i < inputLen ? (unsigned char)input[i++] : 0;
    unsigned int triple = (a << 16) | (b << 8) | c;
    output[j++] = chars[(triple >> 18) & 0x3F];
    output[j++] = chars[(triple >> 12) & 0x3F];
    output[j++] = (i > inputLen + 1) ? '=' : chars[(triple >> 6) & 0x3F];
    output[j++] = (i > inputLen) ? '=' : chars[triple & 0x3F];
  }
  output[j] = '\0';
  return resultNormal(valueString(output));
}

/* ==================== crypto.base64Decode(str) ==================== */

static InterpreterResult cryptoBase64Decode(int argc, RuntimeValue *argv,
                                            RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return cryptoError(error, "base64Decode",
                       "crypto.base64Decode() expects a string");

  const char *input = argv[0].as.string;
  int inputLen = strlen(input);
  int outputLen = inputLen * 3 / 4;
  char *output = gcmall(outputLen + 1);

  int vals[256];
  memset(vals, 0, sizeof(vals));
  const char *chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (int i = 0; i < 64; i++)
    vals[(unsigned char)chars[i]] = i;

  int j = 0;
  for (int i = 0; i < inputLen; i += 4) {
    int a = vals[(unsigned char)input[i]];
    int b = (i + 1 < inputLen) ? vals[(unsigned char)input[i + 1]] : 0;
    int c = (i + 2 < inputLen) ? vals[(unsigned char)input[i + 2]] : 0;
    int d = (i + 3 < inputLen) ? vals[(unsigned char)input[i + 3]] : 0;
    unsigned int triple = (a << 18) | (b << 12) | (c << 6) | d;
    if (j < outputLen)
      output[j++] = (triple >> 16) & 0xFF;
    if (j < outputLen && input[i + 2] != '=')
      output[j++] = (triple >> 8) & 0xFF;
    if (j < outputLen && input[i + 3] != '=')
      output[j++] = triple & 0xFF;
  }
  output[j] = '\0';
  return resultNormal(valueString(output));
}

/* ==================== Module init ==================== */

InterpreterResult stdCryptoInit(Node *node, int id, RuntimeEnv *env,
                               Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  RuntimeValue mod = valueObject(NULL);
  RuntimeValue fn;

  fn = valueNativeFunction("md5", cryptoMd5, 1);
  valueObjectSet(&mod, "md5", fn);

  fn = valueNativeFunction("sha1", cryptoSha1, 1);
  valueObjectSet(&mod, "sha1", fn);

  fn = valueNativeFunction("sha256", cryptoSha256, 1);
  valueObjectSet(&mod, "sha256", fn);

  fn = valueNativeFunction("sha512", cryptoSha512, 1);
  valueObjectSet(&mod, "sha512", fn);

  fn = valueNativeFunction("hmac", cryptoHmac, 2);
  valueObjectSet(&mod, "hmac", fn);

  fn = valueNativeFunction("base64Encode", cryptoBase64Encode, 1);
  valueObjectSet(&mod, "base64Encode", fn);

  fn = valueNativeFunction("base64Decode", cryptoBase64Decode, 1);
  valueObjectSet(&mod, "base64Decode", fn);

  return resultNormal(mod);
}
