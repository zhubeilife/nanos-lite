#include <common.h>
#include "syscall.h"

#include <stdlib.h>

enum {
  SYS_exit,
  SYS_yield,
  SYS_open,
  SYS_read,
  SYS_write,
  SYS_kill,
  SYS_getpid,
  SYS_close,
  SYS_lseek,
  SYS_brk,
  SYS_fstat,
  SYS_time,
  SYS_signal,
  SYS_execve,
  SYS_fork,
  SYS_link,
  SYS_unlink,
  SYS_wait,
  SYS_times,
  SYS_gettimeofday
};

// TODO: it seems not get it from nemu
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
      if (fd == 1 || fd == 2) {
        // fd是1或2(分别代表stdout和stderr)
        char* buf = (char*)c->GPR3;
        size_t count = c->GPR4;
        for (int i = 0; i < count; i++) {
          putch(buf[i]);
        }
        c->GPRx = count;
      }
      else {
        panic("unsupported syscall sys_write with fd: %d", fd);
      }
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
