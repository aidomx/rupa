#include <rupa.h>

bool readfile(const char *path, Buffer *buffer) {
  if (!path || !buffer || !buffer->value || buffer->capacity < 1) return false;

  FILE *file = fopen(path, "rb");
  if (!file) {
    char *p = gcdup(path);
    getcwd(p, MAX_PATH_LENGTH);
    fprintf(stderr, "Error: file %s/%s is not found\n", p, path);
    return false;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return false;
  }

  long end = ftell(file);
  if (end < 0) {
    fclose(file);
    return false;
  }

  size_t fileSize = (size_t)end;
  rewind(file);

  if (fileSize >= (size_t)buffer->capacity) {
    fprintf(stderr, "File %s is greater than buffer size.\n", path);
    fclose(file);
    return false;
  }

  size_t bytes = fread(buffer->value, 1, fileSize, file);
  if (bytes != fileSize) {
    fprintf(stderr, "Error reading file %s\n", path);
    fclose(file);
    return false;
  }

  buffer->value[bytes] = '\0';
  buffer->length = (int)bytes;
  fclose(file);
  return true;
}
