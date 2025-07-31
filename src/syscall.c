#include <common.h>
#include "syscall.h"
#include <fs.h>
#include <sys/time.h>
#include <stdlib.h>
#include <proc.h>

// TODO: it seems not get it from nemu
// #define CONFIG_STRACE
#ifdef CONFIG_STRACE
#define STRACE_LOG(...) Log(__VA_ARGS__)
#else
#define STRACE_LOG(...)
#endif

int get_am_uptime(struct timeval *tv);
int mm_brk(uintptr_t brk);
void naive_uload(PCB *pcb, const char *filename);
char* get_fd_name(int fd);

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_exit : {
      STRACE_LOG("[strace] syscall: SYS_exit, input:%d, output:%d\n", c->GPR2, c->GPRx);
      naive_uload(NULL, "/bin/menu");
      // halt(c->GPR2);
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
      STRACE_LOG("[strace] syscall: SYS_wirte %d %d\n", fd, count);
      int fd = c->GPR2;
      void* buf = (void*)c->GPR3;
      size_t count = c->GPR4;
      c->GPRx = fs_write(fd, buf, count);
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
      STRACE_LOG("[strace] syscall: SYS_lseek %d %d\n", fd, offset);
      c->GPRx = fs_lseek(fd, offset, whence);
      break;
    }
    case SYS_brk: {
      c->GPRx = mm_brk(c->GPR2);
      STRACE_LOG("[strace] syscall: SYS_brk, input: %x, output: %x\n", c->GPR2, c->GPRx);
      break;
    }
    case SYS_gettimeofday: {
      struct timeval *tv = (struct timeval *)c->GPR2;
      c->GPRx = get_am_uptime(tv);
      break;
    }
    case SYS_execve: {
      char* fname = (char*)c->GPR2;
      STRACE_LOG("[strace] syscall: SYS_execve %s\n", fname);
      naive_uload(NULL, fname);
      c->GPRx = 0;
      break;
    }
    default: panic("[strace] Unhandled syscall ID = %d", a[0]);
  }
}
