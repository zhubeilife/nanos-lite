#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <common.h>

// copy from https://mcuoneclipse.com/2021/01/23/assert-__file__-path-and-other-cool-gnu-gcc-tricks-to-be-aware-of/
// #define __FILENAME__ (__builtin_strrchr("/"__FILE__, '/') + 1)
// or just use gcc __FILE_NAME__  https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html

#define Log(format, ...) \
  printf("\33[1;35m[nano-lite: %s,%d,%s] " format "\33[0m\n", \
      __FILE_NAME__, __LINE__, __func__, ## __VA_ARGS__)

#undef panic
#define panic(format, ...) \
  do { \
    Log("\33[1;31msystem panic: " format, ## __VA_ARGS__); \
    halt(1); \
  } while (0)

#ifdef assert
# undef assert
#endif

#define assert(cond) \
  do { \
    if (!(cond)) { \
      panic("Assertion failed: %s", #cond); \
    } \
  } while (0)

#define TODO() panic("please implement me")

#endif
