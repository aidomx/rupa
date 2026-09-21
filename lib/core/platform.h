#pragma once

#define RUPA_VERSION "1.0"

#if defined(_WIN32) || defined(_WIN64)
#include "platform/windows.h"
#define RUPA_WINDOWS 1
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include "platform/posix.h"
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
#else
#define RUPA_SLEEP(ms) usleep((ms) * 1000)
#define RUPA_GETCWD(buf, size) getcwd(buf, size)
#define RUPA_MKDIR(path) mkdir(path, 0755)
#define RUPA_UNLINK(path) unlink(path)
#define RUPA_RENAME(old, new) rename(old, new)
#define RUPA_IS_PATH_SEP(c) ((c) == '/')
#endif
