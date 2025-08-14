#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
PCB *user_pcb = &pcb[1];

void naive_uload(PCB *pcb, const char *filename);

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  int index = 0;
  while (1) {
    if (index % 1000 == 0) {
      Log("hello_fun arg: '%d', %d time!", (int)arg, j);
      j++;
    }
    index++;
    yield();
  }
}

void context_kload(PCB* pcb, void (*entry)(void *), void* arg) {
  Area kstack = {pcb->stack, pcb->stack + STACK_SIZE};
  pcb->cp = kcontext(kstack, entry, arg);
}

uintptr_t loader(PCB *pcb, const char *filename);

/*
|               |
+---------------+ <---- ustack.end
|  Unspecified  |
+---------------+
|               | <----------+
|    string     | <--------+ |
|     area      | <------+ | |
|               | <----+ | | |
|               | <--+ | | | |
+---------------+    | | | | |
|  Unspecified  |    | | | | |
+---------------+    | | | | |
|     NULL      |    | | | | |
+---------------+    | | | | |
|    ......     |    | | | | |
+---------------+    | | | | |
|    envp[1]    | ---+ | | | |
+---------------+      | | | |
|    envp[0]    | -----+ | | |
+---------------+        | | |
|     NULL      |        | | |
+---------------+        | | |
| argv[argc-1]  | -------+ | |
+---------------+          | |
|    ......     |          | |
+---------------+          | |
|    argv[1]    | ---------+ |
+---------------+            |
|    argv[0]    | -----------+
+---------------+
|      argc     |
+---------------+ <---- cp->GPRx
|               |
 */
void context_uload(PCB* pcb, const char *filename, char *const argv[], char *const envp[]) {
  Log("context_uload: execve %s argv:%p envp:%p\n", filename, argv, envp);

  // Calculate total size needed
  int argc = 0, envc = 0;
  size_t total_string_len = 0;

  // Count argc and calculate string lengths
  if (argv != NULL) {
    while (argv[argc] != NULL) {
      total_string_len += strlen(argv[argc]) + 1;
      // Log("argv[%d](addr: %p): %s", argc, argv[argc], argv[argc]);
      argc++;
    }
  }

  if (envp != NULL) {
    while (envp[envc] != NULL) {
      total_string_len += strlen(envp[envc]) + 1;
      // Log("envp[%d](addr: %p): %s", envc, envp[envc], envp[envc]);
      envc++;
    }
  }
  Log("argc size: %d, env size: %d", argc, envc);

  // Calculate total size needed for the stack frame
  // Layout (low -> high):
  // [argc (1 slot of uintptr_t)] [argv pointers ... NULL] [envp pointers ... NULL] [strings]
  size_t frame_size = sizeof(uintptr_t) * (1                  /* argc slot */
                                          + argc              /* argv (no NULL here) */
                                          + (envc + 1))       /* envp + NULL */
                       + total_string_len;                    /* string data */

  // Align to sizeof(uintptr_t)
  const size_t align_unit = sizeof(uintptr_t);
  frame_size = (frame_size + align_unit - 1) & ~(align_unit - 1);

  Log("frame_size:%d, total_string_len:%d", frame_size, total_string_len);

  // get user stack
  void *user_start = new_page(8);
  void *user_end = user_start + 8*PGSIZE;
  Log("Heap area [%p ~ %p]", heap.start, heap.end);
  Log("User Heap area [%p ~ %p]", user_start, user_end);

  // Check if we have enough memory
  if ((char*)user_end - (char*)user_start < frame_size) {
    panic("Not enough memory for process arguments");
  }

  // Place the frame at the top of heap, aligned down
  char* top = (char*)(((uintptr_t)user_end - frame_size) & ~((uintptr_t)align_unit - 1));
  Log("top: %p", top);

  // Compute pointers for each region
  uintptr_t *args_base = (uintptr_t *)top;              // points to argc slot
  uintptr_t *argv_ptrs = args_base + 1;                 // immediately after argc
  uintptr_t *envp_ptrs = argv_ptrs + argc;              // directly after argv (no NULL)
  char *string_base = (char *)(envp_ptrs + (envc + 1)); // after envp and its NULL

  Log("argc_ptr:%p argv_ptr:%p envp_ptr:%p string_base:%p", args_base, argv_ptrs, envp_ptrs, string_base);

  // Copy argv strings and fill argv pointers
  char* current_string = string_base;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    strcpy(current_string, argv[i]);
    argv_ptrs[i] = (uintptr_t)current_string;
    current_string += len;
  }
  // Copy envp strings and fill envp pointers
  for (int i = 0; i < envc; i++) {
    size_t len = strlen(envp[i]) + 1;
    strcpy(current_string, envp[i]);
    envp_ptrs[i] = (uintptr_t)current_string;
    current_string += len;
  }
  envp_ptrs[envc] = 0;  // NULL terminator

  // Set argc in the first slot (pointer-sized)
  args_base[0] = (uintptr_t)argc;

  // Log("args base %p, user_end %p", args_base, user_end);
  // for (int i = 0; i < argc; i++) {
  //   Log("argv[%d](addr: %p): %s", i, (void*)argv_ptrs[i], (char*)argv_ptrs[i]);
  // }

  uintptr_t entry = loader(pcb, filename);

  if (entry == 0){
    Log("Fail to load %s", filename);
    panic("Fail to load %s", filename);
    return;
  }
  Area kstack = {pcb->stack, pcb->stack + STACK_SIZE};
  pcb->cp = ucontext(NULL, kstack, (void*)entry);
  // Pass args base in a0; user _start will set sp from a0 and call call_main
  pcb->cp->GPRx = (uintptr_t)args_base;

  Log("End params load");
}

void init_proc() {
  char *envp[] = {NULL};
  context_kload(&pcb[0], hello_fun, (void *)1);
  // context_kload(&pcb[1], hello_fun, (void *)2);
  // char *argv[] = {"/bin/pal", "--skip", NULL};
  // char *argv[] = {"/bin/exec-test", "hello", NULL};
  // char *argv[] = {"/bin/menu", NULL, NULL};
  char *argv[] = {"/bin/nterm", NULL, NULL};
  // char *argv[] = {"/bin/busybox", NULL, NULL};
  // char *argv[] = {"/usr/bin/basename", "/h/a/ddfdf", NULL};
  context_uload(&pcb[1], argv[0], argv, envp);
  switch_boot_pcb();

  Log("Initializing processes...");

  // load program here
  // naive_uload(NULL, "/bin/menu");
}

Context* schedule(Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  return current->cp;
}
