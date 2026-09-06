#pragma once
#define RUPA_VERSION "1.0"

/* ============================================
 * Platform-specific Headers
 * ============================================
 * Uncomment headers as needed for the project.
 * Headers marked [USED] are currently in use.
 */

/* --------------------------------------------
 * Windows Platform
 * --------------------------------------------
 */
#if defined(_WIN32) || defined(_WIN64)
#define RUPA_WINDOWS 1

/* --- Core Windows --- */
#include <windows.h>        /* [USED] Windows API (base, file, process, etc.) */
#include <conio.h>          /* [USED] Console I/O (getch, kbhit) */
#include <io.h>             /* [USED] Low-level I/O (access, open, close) */

/* --- File System --- */
// #include <direct.h>       /* Directory functions (mkdir, rmdir, getcwd) */
// #include <shlobj.h>       /* Shell object (SHGetFolderPath) */
// #include <shellapi.h>     /* Shell API (ShellExecute) */
// #include <shlwapi.h>      /* Shell Lightweight API (PathCombine, etc.) */
// #include <winbase.h>      /* Base API (CreateFile, etc.) */
// #include <windef.h>       /* Windows type definitions */
// #include <winerror.h>     /* Windows error codes */
// #include <winnls.h>       /* National Language Support */

/* --- Process/Thread --- */
// #include <process.h>      /* Process control (_beginthread, _getpid) */
// #include <processthreadsapi.h> /* Process/thread creation */
// #include <synchapi.h>     /* Synchronization (Critical Section, Mutex) */
// #include <handleapi.h>    /* Handle management (CloseHandle, etc.) */

/* --- Networking --- */
// #include <winsock2.h>     /* Windows Sockets (WSAStartup, socket, etc.) */
// #include <ws2tcpip.h>     /* TCP/IP Winsock extensions */
// #include <mswsock.h>      /* Microsoft-specific Winsock extensions */

/* --- Registry --- */
// #include <winreg.h>       /* Registry functions (RegOpenKey, RegSetValue) */

/* --- Memory --- */
// #include <memoryapi.h>    /* VirtualAlloc, VirtualFree */
// #include <heapapi.h>      /* Heap management */

/* --- Debugging --- */
// #include <debugapi.h>     /* OutputDebugString, IsDebuggerPresent */
// #include <dbghelp.h>      /* Debug Help Library (stack traces) */
// #include <errhandlingapi.h> /* GetLastError, SetLastError */

/* --- Services --- */
// #include <winsvc.h>       /* Windows Services */
// #include <tlhelp32.h>     /* Tool Help (Process/Module/Thread snapshots) */

/* --- COM/DDE --- */
// #include <objbase.h>      /* COM initialization */
// #include <commdlg.h>      /* Common dialogs (Open/Save file) */

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

/* --------------------------------------------
 * POSIX Platform (Linux, macOS, BSD, etc.)
 * --------------------------------------------
 */
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define RUPA_POSIX 1

/* --- Core POSIX --- */
#include <unistd.h>         /* [USED] POSIX API (read, write, fork, etc.) */
#include <sys/types.h>      /* [USED] System data types */

/* --- Terminal I/O --- */
#include <termios.h>        /* [USED] Terminal I/O (tcgetattr, tcsetattr) */
#include <sys/ioctl.h>      /* [USED] Terminal I/O control (ioctl) */
// #include <ncurses.h>      /* ncurses terminal UI library (third-party) */

/* --- File System --- */
#include <dirent.h>         /* [USED] Directory operations */
// #include <fcntl.h>        /* File control (open, fcntl) — moved to stdlib.h */
// #include <sys/statvfs.h>  /* Filesystem statistics */
// #include <sys/sendfile.h> /* Efficient file transfer (Linux) */

/* --- Process --- */
// #include <sys/wait.h>     /* Wait for process termination */
// #include <sys/resource.h> /* Resource usage (getrusage) */
// #include <sys/syscall.h>  /* System call numbers */
// #include <spawn.h>        /* posix_spawn */

/* --- Signals --- */
// #include <signal.h>       /* Signal handling */
// #include <sys/signalfd.h> /* Signal file descriptor (Linux) */
// #include <sys/timerfd.h>  /* Timer file descriptor (Linux) */
// #include <sys/eventfd.h>  /* Event file descriptor (Linux) */

/* --- IPC --- */
// #include <sys/ipc.h>      /* IPC */
// #include <sys/shm.h>      /* Shared memory */
// #include <sys/mman.h>     /* Memory-mapped files */
// #include <sys/sem.h>      /* Semaphores */
// #include <sys/msg.h>      /* Message queues */
// #include <mqueue.h>       /* POSIX message queues */

/* --- Networking --- */
// #include <sys/socket.h>   /* Socket programming */
// #include <netinet/in.h>   /* Internet address family */
// #include <arpa/inet.h>    /* Internet operations */
// #include <netdb.h>        /* Network database */
// #include <sys/un.h>       /* UNIX domain sockets */
// #include <netinet/tcp.h>  /* TCP options */

/* --- Thread Support --- */
// #include <pthread.h>      /* POSIX threads */
// #include <semaphore.h>    /* Semaphores */
// #include <sys/prctl.h>    /* Process control (Linux) */

/* --- Polling --- */
// #include <poll.h>         /* I/O multiplexing (poll) */
// #include <sys/epoll.h>    /* I/O event notification (Linux) */
// #include <sys/select.h>   /* I/O multiplexing (select) */
// #include <sys/event.h>    /* I/O event notification (BSD kqueue) */

/* --- Time --- */
// #include <sys/time.h>     /* Time structures — moved to stdlib.h */
// #include <time.h>         /* Time functions — moved to stdlib.h */
// #include <sys/times.h>    /* Process times */
// #include <time.h>         /* Clock functions (clock_gettime) */

/* --- Dynamic Loading --- */
// #include <dlfcn.h>        /* Dynamic linking (dlopen, dlsym) */

/* --- User/Group --- */
// #include <pwd.h>          /* User database (getpwnam) */
// #include <grp.h>          /* Group database (getgrnam) */
// #include <sys/utsname.h>  /* System name — moved to stdlib.h */

/* --- Logging --- */
// #include <syslog.h>       /* System logging (syslog, openlog) */

/* --- Regex --- */
// #include <regex.h>        /* POSIX regular expressions */

/* --- Capability (Linux) --- */
// #include <sys/capability.h> /* Linux capabilities */

/* --- Inotify (Linux) --- */
// #include <sys/inotify.h>  /* File system event monitoring (Linux) */

/* --- Udev (Linux) --- */
// #include <libudev.h>      /* Device management (Linux, third-party) */

/* --- ACL --- */
// #include <sys/acl.h>      /* Access Control Lists (POSIX.1e) */

/* --- Extended Attributes --- */
// #include <sys/xattr.h>    /* Extended attributes (Linux/BSD) */

/* --- Sandbox (macOS) --- */
// #include <sandbox.h>      /* macOS sandbox */

/* --- Kernel Event (BSD/macOS) --- */
// #include <sys/event.h>    /* kqueue event notification */

/* --- Random --- */
// #include <sys/random.h>   /* Random number generator (Linux/BSD) */

/* --- seccomp (Linux) --- */
// #include <seccomp.h>      /* Seccomp filtering (Linux) */

/* --- FUSE (Linux) --- */
// #include <fuse.h>         /* Filesystem in Userspace (Linux, third-party) */

#else
#warning "Unsupported platform - PR welcome for platform support"
#endif

/* ============================================
 * Platform Macros
 * ============================================
 */
#if RUPA_WINDOWS
#define RUPA_SLEEP(ms) Sleep(ms)
#define RUPA_GETCWD(buf, size) _getcwd(buf, size)
#define RUPA_MKDIR(path) _mkdir(path)
#define RUPA_UNLINK(path) _unlink(path)
#define RUPA_RENAME(old, new) RenameFile(old, new)
#define RUPA_IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
#else
#define RUPA_SLEEP(ms) usleep((ms) * 1000)
#define RUPA_GETCWD(buf, size) getcwd(buf, size)
#define RUPA_MKDIR(path) mkdir(path, 0755)
#define RUPA_UNLINK(path) unlink(path)
#define RUPA_RENAME(old, new) rename(old, new)
#define RUPA_IS_PATH_SEP(c) ((c) == '/')
#endif
