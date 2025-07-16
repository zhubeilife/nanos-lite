#include <common.h>
#include "syscall.h"
#include <fs.h>

#include <stdlib.h>

// TODO: it seems not get it from nemu
// #define CONFIG_STRACE
#ifdef CONFIG_STRACE
#define STRACE_LOG(...) Log(__VA_ARGS__)
#else
#define STRACE_LOG(...)
#endif

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_exit : {
      STRACE_LOG("[strace] syscall: SYS_exit, input:%d, output:%d\n", c->GPR2, c->GPRx);
      halt(c->GPR2);
      c->GPRx = 0;
      break;
    }
    case SYS_yield: {
      STRACE_LOG("[strace] syscall: SYS_yield, input:N/A, output:%d\n", c->GPRx);
      yield();
      c->GPRx = 0;
      break;
    }
    case SYS_write: {
      STRACE_LOG("[strace] syscall: SYS_wirte\n");
      int fd = c->GPR2;
      if (fd == 0) {
        c->GPRx = 0;
      }
      else if (fd == 1 || fd == 2) {
        // fd是1或2(分别代表stdout和stderr)
        char* buf = (char*)c->GPR3;
        size_t count = c->GPR4;
        for (int i = 0; i < count; i++) {
          putch(buf[i]);
        }
        c->GPRx = count;
      }
      else {
        void* buf = (void*)c->GPR3;
        size_t count = c->GPR4;
        c->GPRx = fs_write(fd, buf, count);
      }
      break;
    }
    case SYS_open: {
      char *pathname = (char*)c->GPR2;
      int flags = c->GPR3;
      int mode = c->GPR4;
      c->GPRx = fs_open(pathname, flags, mode);
      break;
    }
    case SYS_read: {
      int fd = c->GPR2;
      void *buf = (void*)c->GPR3;
      size_t len = c->GPR4;
      c->GPRx = fs_read(fd, buf, len);
      break;
    }
    case SYS_close: {
      int fd = c->GPR2;
      STRACE_LOG("[strace] syscall: SYS_close %d\n", fd);
      c->GPRx = fs_close(fd);
      break;
    }
    case SYS_lseek: {
      int fd = c->GPR2;
      int offset = c->GPR3;
      int whence = c->GPR4;
      c->GPRx = fs_lseek(fd, offset, whence);
      break;
    }
    case SYS_brk: {
      intptr_t addr = c->GPR1;
      STRACE_LOG("[strace] syscall: SYS_brk, input:%d, output:%d\n", addr,c->GPRx);
      c->GPRx = 0;
      break;
    }
    default: panic("[strace] Unhandled syscall ID = %d", a[0]);
  }
}
