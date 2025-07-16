#include <proc.h>
#include <elf.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);

#if defined(__ISA_NATIVE__)
# define EXPECT_TYPE EM_ARM
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV
#else
#error Unsupported ISA
#endif

/*
*ELF Header:
  Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF32
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           RISC-V
  Version:                           0x1
  Entry point address:               0x80000000
  Start of program headers:          52 (bytes into file)
  Start of section headers:          13716 (bytes into file)
  Flags:                             0x0
  Size of this header:               52 (bytes)
  Size of program headers:           32 (bytes)
  Number of program headers:         3
  Size of section headers:           40 (bytes)
  Number of section headers:         12
  Section header string table index: 11
*/
void read_elf_header(Elf_Ehdr *elf_header)
{
  ramdisk_read(elf_header, 0, sizeof(Elf_Ehdr));
}

void read_elf_header_fd(Elf_Ehdr *elf_header, int fd)
{
  fs_read(fd, elf_header, sizeof(Elf_Ehdr));
}

void check_elf_header(Elf_Ehdr *elf_header)
{
  // check magic number
  if(!strncmp((char*)elf_header->e_ident, "\177ELF", 4)) {
    // printf("ELFMAGIC \t= ELF\n");
    /* IS a ELF file */
  }
  else {
    panic("ELFMAGIC mismatch!\n");
    /* Not ELF file */
  }
  // check the arch
  if ((elf_header->e_machine != EXPECT_TYPE)) {
    panic("wrong machine code with %d", elf_header->e_machine);
  }
}

void read_segment_header_table(Elf_Ehdr *elf_header, Elf_Phdr ph_table[])
{
  ramdisk_read(ph_table, elf_header->e_phoff, sizeof(Elf_Phdr) * elf_header->e_phnum);
}

void read_segment_header_table_fd(Elf_Ehdr *elf_header, Elf_Phdr ph_table[], size_t fd)
{
  fs_lseek(fd, elf_header->e_phoff, SEEK_SET);
  fs_read(fd, ph_table, sizeof(Elf_Phdr) * elf_header->e_phnum);
}

void print_segment_headers(Elf_Ehdr *elf_header, Elf_Phdr ph_table[])
{
  /*
  Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  RISCV_ATTRIBUT 0x0058b7 0x00000000 0x00000000 0x0004e 0x00000 R   0x1
  LOAD           0x000000 0x83000000 0x83000000 0x04d40 0x04d40 R E 0x1000
  LOAD           0x005000 0x83005000 0x83005000 0x00898 0x008d4 RW  0x1000
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x00000 RW  0x10
   */
  printf("========================================");
  printf("========================================\n");
  printf("Program Headers:\n");
  printf("Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align\n");
  printf("========================================");
  printf("========================================\n");

  for(uint32_t i = 0; i < elf_header->e_phnum; i++) {
    printf("%u ", ph_table[i].p_type);
    printf("%u ", ph_table[i].p_offset);
    printf("%u ", ph_table[i].p_vaddr);
    printf("%u ", ph_table[i].p_paddr);
    printf("%u ", ph_table[i].p_filesz);
    printf("%u ", ph_table[i].p_memsz);
    printf("%u ", ph_table[i].p_flags);
    printf("%u ", ph_table[i].p_align);
    printf("\n");
  }
  printf("========================================");
  printf("========================================\n");
  printf("\n");	/* end of section header table */
}

void load_program(Elf_Ehdr *elf_header, Elf_Phdr ph_table[]) {
  for(uint32_t i = 0; i < elf_header->e_phnum; i++) {
    if (ph_table[i].p_type == PT_LOAD) {
      // [VirtAddr, VirtAddr + MemSiz)
      ramdisk_read((void*)ph_table[i].p_vaddr, ph_table[i].p_offset, ph_table[i].p_filesz);
      // [VirtAddr + FileSiz, VirtAddr + MemSiz)
      memset((void*)(ph_table[i].p_vaddr + ph_table[i].p_filesz) , 0, ph_table[i].p_memsz - ph_table[i].p_filesz);
    }
  }
}

void load_program_fd(Elf_Ehdr *elf_header, Elf_Phdr ph_table[], int fd) {
  for(uint32_t i = 0; i < elf_header->e_phnum; i++) {
    if (ph_table[i].p_type == PT_LOAD) {
      // [VirtAddr, VirtAddr + MemSiz)
      fs_lseek(fd, ph_table[i].p_offset, SEEK_SET);
      fs_read(fd, (void*)ph_table[i].p_vaddr, ph_table[i].p_filesz);
      // [VirtAddr + FileSiz, VirtAddr + MemSiz)
      memset((void*)(ph_table[i].p_vaddr + ph_table[i].p_filesz) , 0, ph_table[i].p_memsz - ph_table[i].p_filesz);
    }
  }
}

static uintptr_t loader(PCB *pcb, const char *filename) {

  // -1- open the file
  int fd = fs_open(filename, 0, 0);
  if (fd < 0) {
    panic("could not open file %s", filename);
  }
  // 0- read the elf header
  Elf_Ehdr elf_header;
  read_elf_header_fd(&elf_header, fd);
  // 1- check the elf header
  check_elf_header(&elf_header);
  // 2- read segment table
  /* section-header table is variable size */
  Elf_Phdr* ph_tbl = malloc(sizeof(Elf_Phdr) * elf_header.e_phnum);
  read_segment_header_table_fd(&elf_header, ph_tbl, fd);
  print_segment_headers(&elf_header, ph_tbl);
  // 3- load to memory
  load_program_fd(&elf_header, ph_tbl, fd);
  return elf_header.e_entry;

/*  PA3.2 directly load one elf file
  // 0- read the elf header
  Elf_Ehdr elf_header;
  read_elf_header(&elf_header);
  // 1- check the elf header
  check_elf_header(&elf_header);
  // 2- read segment table
  // section-header table is variable size//
  Elf_Phdr* ph_tbl = malloc(sizeof(Elf_Phdr) * elf_header.e_phnum);
  read_segment_header_table(&elf_header, ph_tbl);
  print_segment_headers(&elf_header, ph_tbl);
  // 3- load to memory
  load_program(&elf_header, ph_tbl);
  return elf_header.e_entry;
 */
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  // TODO printf not support %p for now
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

