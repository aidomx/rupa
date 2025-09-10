#include <rupa/package.h>

void clearScreen() {
#ifdef _WIN32
  system("cls")
#else
  system("clear");
#endif
}

void replace(const char *str, const char *key, const char *value,
             char output[]) {
  if (!str || !key || !value || !output)
    return;

  char *p = strstr(str, key);
  if (!p) {
    strncpy(output, str, MAX_MESSAGE_LENGTH - 1);
    output[MAX_MESSAGE_LENGTH - 1] = '\0';
    return;
  }

  int prefix_len = p - str;
  int remaining_len = MAX_MESSAGE_LENGTH - 1;

  snprintf(output, remaining_len, "%.*s%s%s", prefix_len, str, value,
           p + strlen(key));
}

char *serialize(const char *str, char buffer[]) {
  int newline = 0, out = 0;

  while (*str != '\0') {
    if (isnewline(*str)) {
      if (!newline) {
        buffer[out++] = '\x1F';
        newline = 1;
      }
    } else {
      buffer[out++] = *str;
      newline = 0;
    }
    str++;
  }
  buffer[out] = '\0';

  return buffer;
}

char *substring(const char *input, int start, int end) {
  if (!input || start < 0 || end <= start)
    return NULL;

  int len = end - start;
  char *result = malloc(len + 1);
  if (!result)
    return NULL;

  strncpy(result, input + start, len);
  result[len] = '\0';

  return result;
}

void trimbracket(char *value, char open, char close) {
  if (!value)
    return;

  if (open == '(') {
    size_t n = strlen(value);
    memmove(value, value + 1, n);
    value[n] = '\0';
  } else if (close == ')') {
    size_t n = strlen(value) - 1;
    memmove(value, value, n);
    value[n] = '\0';
  }
}

char *trimspace(char *value) {
  if (value == NULL || *value == '\0')
    return value;

  char *dest = value;
  char *str = value;
  int in_quote = 0;
  char quote_char = '\0';

  while (*str) {
    if (isquote(*str)) {
      if (!in_quote) {
        in_quote = 1;
        quote_char = *str;
      }

      else if (in_quote && *str == quote_char) {
        in_quote = 0;
      }

      *dest++ = *str++;
      continue;
    }

    if (in_quote) {
      *dest++ = *str++;
    }

    else {
      if (!isspace((unsigned char)*str)) {
        *dest++ = *str;
      }
      str++;
    }
  }

  *dest = '\0';
  return value;
}

void trimquote(char *value) {
  int n = strlen(value) - 1;

  if (isquote(value[0]) && isquote(value[n])) {
    memmove(value, value + 1, n);
    value[n - 1] = '\0';
  } else {
    n = strlen(value) - 2;

    if (isquote(value[0]) && isquote(value[n])) {
      memmove(value, value + 1, n);
      value[n - 1] = '\0';
    }
  }
}
