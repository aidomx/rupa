#pragma once
#define RUPA_VERSION "1.0"

#if defined(_WIN32) || defined(_WIN64)
#define RUPA_WINDOWS 1

#include <conio.h>
#include <io.h>
#include <windows.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define RUPA_POSIX 1

#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#else
#warning "Unsupported platform - PR welcome for platform support"
#endif

#if RUPA_WINDOWS
#define RUPA_SLEEP(ms) Sleep(ms)
#else
#define RUPA_SLEEP(ms) usleep((ms) * 1000)
#endif
