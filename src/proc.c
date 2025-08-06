#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void naive_uload(PCB *pcb, const char *filename);

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%d' for the %dth time!", (int)arg, j);
    j ++;
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
  uintptr_t entry = loader(pcb, filename);
  if (entry == 0){
    Log("Fail to load %s", filename);
    panic("Fail to load %s", filename);
    return;
  }
  Area kstack = {pcb->stack, pcb->stack + STACK_SIZE};
  pcb->cp = ucontext(NULL, kstack, (void*)entry);

  // Calculate total size needed
  int argc = 0, envc = 0;
  size_t total_string_len = 0;
  
  // Count argc and calculate string lengths
  if (argv != NULL) {
    while (argv[argc] != NULL) {
      total_string_len += strlen(argv[argc]) + 1;
      argc++;
    }
  }
  
  if (envp != NULL) {
    while (envp[envc] != NULL) {
      total_string_len += strlen(envp[envc]) + 1;
      envc++;
    }
  }
  
  // Calculate total size needed for the stack frame
  size_t frame_size = sizeof(uintptr_t) * (argc + 1) +  // argv array + NULL
                      sizeof(uintptr_t) * (envc + 1) +  // envp array + NULL
                      sizeof(int) +                      // argc
                      total_string_len;                 // string data
  
  // Align to 8-byte boundary
  frame_size = (frame_size + 7) & ~7;
  
  // Check if we have enough memory
  if ((char*)heap.end - (char*)heap.start < frame_size) {
    panic("Not enough memory for process arguments");
  }
  
  // Start from the end of heap
  char* top = (char*)heap.end - frame_size;
  
  // Ensure 8-byte alignment
  top = (char*)((uintptr_t)top & ~7);
  
  // Copy strings first (from bottom to top)
  char* string_base = top + sizeof(uintptr_t) * (argc + 1) + 
                      sizeof(uintptr_t) * (envc + 1) + sizeof(int);
  
  uintptr_t* argv_ptrs = (uintptr_t*)top;
  uintptr_t* envp_ptrs = (uintptr_t*)(top + sizeof(uintptr_t) * (argc + 1));
  int* argc_ptr = (int*)(top + sizeof(uintptr_t) * (argc + 1) + sizeof(uintptr_t) * (envc + 1));
  
  // Copy argv strings
  char* current_string = string_base;
  for (int i = 0; i < argc; i++) {
    int len = strlen(argv[i]) + 1;
    strcpy(current_string, argv[i]);
    argv_ptrs[i] = (uintptr_t)current_string;
    current_string += len;
  }
  argv_ptrs[argc] = 0;  // NULL terminator
  
  // Copy envp strings
  for (int i = 0; i < envc; i++) {
    int len = strlen(envp[i]) + 1;
    strcpy(current_string, envp[i]);
    envp_ptrs[i] = (uintptr_t)current_string;
    current_string += len;
  }
  envp_ptrs[envc] = 0;  // NULL terminator
  
  // Set argc
  *argc_ptr = argc;
  
  // Set the stack pointer for the new context
  pcb->cp->GPRx = (uintptr_t)argc_ptr;
}

void init_proc() {
  context_kload(&pcb[0], hello_fun, (void *)1);
  // context_kload(&pcb[1], hello_fun, (void *)2);
  char * pal_argv[] = {"--skip"};
  context_uload(&pcb[1], "/bin/pal", pal_argv, NULL);
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
