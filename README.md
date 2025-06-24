# Nanos-lite

Nanos-lite is the simplified version of Nanos (http://cslab.nju.edu.cn/opsystem).
It is ported to the [AM project](https://github.com/NJU-ProjectN/abstract-machine.git).
It is a two-tasking operating system with the following features
* ramdisk device drivers
* ELF program loader
* memory management with paging
* a simple file system
  * with fix number and size of files
  * without directory
  * some device files
* 9 system calls
  * open, read, write, lseek, close, gettimeofday, brk, exit, execve
* scheduler with two tasks

```txt
nanos-lite
├── include
│   ├── common.h
│   ├── debug.h
│   ├── fs.h
│   ├── memory.h
│   └── proc.h
├── Makefile
├── README.md
├── resources
│   └── logo.txt    # Project-N logo文本
└── src
    ├── device.c    # 设备抽象
    ├── fs.c        # 文件系统
    ├── irq.c       # 中断异常处理
    ├── loader.c    # 加载器
    ├── main.c
    ├── mm.c        # 存储管理
    ├── proc.c      # 进程调度
    ├── ramdisk.c   # ramdisk驱动程序
    ├── resources.S # ramdisk内容和Project-N logo
    └── syscall.c   # 系统调用处理
```