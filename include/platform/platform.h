#pragma once

#define RUPA_VERSION "1.0"

#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"
#define RUPA_WINDOWS 1
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include "posix.h"
#define RUPA_POSIX 1
#else
#error "Unsupported platform - PR welcome for platform support"
#endif

#if defined(RUPA_WINDOWS)
#define RUPA_SLEEP(ms) Sleep(ms)
#define RUPA_GETCWD(buf, size) _getcwd(buf, size)
#define RUPA_MKDIR(path) _mkdir(path)
#define RUPA_UNLINK(path) _unlink(path)
#define RUPA_RENAME(old, new) rename(old, new)
#define RUPA_IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
/* Path kanonik (resolusi symlink & '..') untuk deteksi circular import.
 * Return buffer ter-alokasi atau NULL; caller yang membebaskan.
 * GNU-specific resolvepath() dihindari demi portability MinGW/clang. */
#define RUPA_REALPATH(path) _fullpath(NULL, (path), 0)
#else
#define RUPA_SLEEP(ms) usleep((ms) * 1000)
#define RUPA_GETCWD(buf, size) getcwd(buf, size)
#define RUPA_MKDIR(path) mkdir(path, 0755)
#define RUPA_UNLINK(path) unlink(path)
#define RUPA_RENAME(old, new) rename(old, new)
#define RUPA_IS_PATH_SEP(c) ((c) == '/')
/* Path kanonik (resolusi symlink & '..') untuk deteksi circular import.
 * resolved_path = NULL → buffer malloc() oleh realpath(), caller yang
 * membebaskan (perilaku baku POSIX.1-2008). */
#define RUPA_REALPATH(path) realpath((path), NULL)
#endif
