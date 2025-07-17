#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENT, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
/*
                file at ramdisk
+-------------+---------+----------+-----------+--
|    file0    |  file1  |  ......  |   filen   |
+-------------+---------+----------+-----------+--
 \           / \       /            \         /
  +  size0  +   +size1+              + sizen +

*/
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, serial_write},
  [FD_EVENT] = {"/dev/events", 0, 0, 0, events_read, invalid_write},
#include "files.h"
};

#define FILE_NUMS sizeof(file_table) / sizeof(Finfo)

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

// 为了简化实现, 我们允许所有用户程序都可以对所有已存在的文件进行读写, 这样以后, 我们在实现fs_open()的时候就可以忽略flags和mode了.
// 在Nanos-lite中, 由于sfs的文件数目是固定的, 我们可以简单地把文件记录表的下标作为相应文件的文件描述符返回给用户程序. 在这以后, 所有文件操作都通过文件描述符来标识文件:
int fs_open(const char *pathname, int flags, int mode) {
  // size_t filenums = sizeof(file_table) / sizeof(Finfo);
  int index = -1;
  for (int i = 0; i < FILE_NUMS; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      index = i;
      file_table[i].open_offset = 0;
    }
  }
  if (index == -1) {
    panic("can't fild file");
  }
  return index;
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert((fd >= 0) && (fd < FILE_NUMS));

  if (file_table[fd].read != NULL) {
    return file_table[fd].read(buf, file_table[fd].open_offset, len);
  }

  if (file_table[fd].open_offset + len > file_table[fd].size) {
    // panic("len is large than file nums");
    return -1;
  }
  ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += len;
  return len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert((fd >= 0) && (fd < FILE_NUMS));

  if (file_table[fd].write != NULL) {
    file_table[fd].write(buf, file_table[fd].open_offset, len);
  }

  if (file_table[fd].open_offset + len > file_table[fd].size) {
    // panic("len is large than file nums");
    return -1;
  }
  ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += len;
  return len;
}

/*
DESCRIPTION
       lseek()  repositions  the  file offset of the open file description associated with the file descriptor fd to the argument offset according to the directive whence as follows:

       SEEK_SET
              The file offset is set to offset bytes.
       SEEK_CUR
              The file offset is set to its current location plus offset bytes.
       SEEK_END
              The file offset is set to the size of the file plus offset bytes.
*/
size_t fs_lseek(int fd, size_t offset, int whence) {
  assert((fd >= 0) && (fd < FILE_NUMS));

  switch (whence) {
    case SEEK_SET:
      if (offset > file_table[fd].size) {
        // panic("len is large than file nums");
        return -1;
      }
      file_table[fd].open_offset = offset;
      return file_table[fd].open_offset;
      break;
    case SEEK_CUR:
      if (file_table[fd].open_offset + offset > file_table[fd].size) {
        // panic("len is large than file nums");
        return -1;
      }
      file_table[fd].open_offset += offset;
      return file_table[fd].open_offset;
      break;
    case SEEK_END:
      if (file_table[fd].size < offset) {
        // panic("len is large than file nums");
        return -1;
      }
      file_table[fd].open_offset = file_table[fd].size - offset;
      return file_table[fd].open_offset;
      break;
    default:
      panic("invalid whence");
  }

  return -1;
}

// 由于sfs没有维护文件打开的状态, fs_close()可以直接返回0, 表示总是关闭成功.
int fs_close(int fd) {

  assert((fd >= 0) && (fd < FILE_NUMS));
  file_table[fd].open_offset = 0;
  return 0;
}
