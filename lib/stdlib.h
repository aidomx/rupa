#pragma once

/* ============================================
 * Standard C Library Headers
 * ============================================
 * Uncomment headers as needed for the project.
 * Headers marked [USED] are currently in use.
 */

/* --- Character Handling --- */
#include <ctype.h>  /* [USED] Character classification and conversion */
#include <wctype.h> /* Wide character classification (wchar_t) */

/* --- String Handling --- */
#include <string.h>  /* [USED] String manipulation functions */
#include <strings.h> /* BSD string functions (bzero, strcasecmp) */
// #include <wchar.h>       /* Wide character string functions */
// #include <uchar.h>       /* Unicode character types (C11) */

/* --- Memory Handling --- */
#include <stdlib.h> /* [USED] Memory allocation, conversion, etc. */
// #include <alloca.h>       /* Stack allocation (non-standard, POSIX) */

/* --- Input/Output --- */
#include <stdio.h> /* [USED] Standard I/O functions */
// #include <wchar.h>       /* Wide character I/O */

/* --- Mathematics --- */
#include <math.h> /* [USED] Mathematical functions */
// #include <complex.h>      /* Complex number arithmetic (C99) */
// #include <fenv.h>         /* Floating-point environment */

/* --- Type Definitions --- */
#include <float.h>    /* Limits of floating-point types */
#include <inttypes.h> /* Format macros for integer types (PRId64, etc.) */
#include <limits.h>   /* [USED] Limits of fundamental types */
#include <stdbool.h>  /* [USED] Boolean type (true, false) */
#include <stddef.h>   /* [USED] Common definitions (size_t, NULL) */
#include <stdint.h>   /* [USED] Fixed-width integer types */
// #include <stdalign.h>     /* Alignment macros (C11) */
// #include <stdnoreturn.h>  /* Noreturn function specifier (C11) */

/* --- Date and Time --- */
#include <sys/time.h>  /* POSIX time structures (gettimeofday) */
#include <time.h>      /* [USED] Date and time functions */
#include <sys/timeb.h> /*ftime() function (legacy) */
#include <bits/wordsize.h>

/* --- Error Handling --- */
#include <errno.h> /* [USED] Error number definitions */
// #include <assert.h>       /* Assert macro */
#include <error.h> /* GNU error handling (non-standard) */

/* --- Dynamic Memory --- */
// #include <malloc.h>       /* Non-standard malloc extensions */
// #include <gc.h>           /* Boehm GC (third-party) */

/* --- Signals --- */
#include <signal.h> /* Signal handling */
// #include <setjmp.h>       /* Non-local jumps (setjmp/longjmp) */
// #include <stdatomic.h>    /* Atomic operations (C11) */
// #include <threads.h>      /* Thread support (C11) */

/* --- Locale --- */
// #include <locale.h>       /* Locale-specific settings */

/* --- Multibyte Characters --- */
// #include <wchar.h>       /* Wide character I/O */
// #include <wctype.h>      /* Wide character classification */

/* --- File System (POSIX) --- */
#include <dirent.h>      /* [USED] Directory entry operations */
#include <fcntl.h>       /* File control options */
#include <libgen.h>      /* Filename parsing (basename, dirname) */
#include <sys/stat.h>    /* [USED] File status */
#include <sys/types.h>   /* [USED] System data types */
#include <sys/utsname.h> /* [USED] System name/OS info */
#include <unistd.h>      /* [USED] POSIX operating system API */
                         // #include <fnmatch.h>      /* Filename pattern matching */
                         // #include <glob.h>         /* Pathname pattern expansion */

/* --- Process Management (POSIX) --- */
#include <sys/wait.h>     /* Wait for process termination */
#include <sys/resource.h> /* Resource usage */
#include <sys/syscall.h>  /* System call numbers */

/* --- IPC (POSIX) --- */
// #include <sys/ipc.h>      /* IPC */
// #include <sys/shm.h>      /* Shared memory */
// #include <sys/mman.h>     /* Memory-mapped files */
#include <sys/socket.h> /* Socket programming */
#include <netinet/in.h> /* Internet address family */
#include <arpa/inet.h>  /* Internet operations */
// #include <netdb.h>        /* Network database operations */

/* --- Terminal I/O (POSIX) --- */
// #include <termios.h>      /* Terminal I/O (moved to platform.h) */
// #include <sys/ioctl.h>    /* Terminal I/O control (moved to platform.h) */

/* --- Thread Support (POSIX) --- */
#include <pthread.h> /* POSIX threads */
// #include <semaphore.h>    /* Semaphores */

/* --- Regex --- */
#include <regex.h> /* POSIX regular expressions */

/* --- Cryptography --- */
/*#include <openssl/sha.h> [> OpenSSL (third-party) <]*/
#include <openssl/evp.h>
/*#include <openssl/md5.h> [> OpenSSL (third-party) <]*/

/* --- Networking --- */
// #include <curl/curl.h> /* libcurl (third-party) */
// #include <sqlite3.h>      /* SQLite (third-party) */

/* --- Debugging --- */
#include <assert.h> /* Assert macro */
// #include <execinfo.h>     /* Backtrace (GNU extension) */
// #include <dlfcn.h>        /* Dynamic linking */
