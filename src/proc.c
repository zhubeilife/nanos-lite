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

  // deal with params
  char* top = (char*)heap.end - 1;
  int string_area_len = 0;
  // argv是一个以NULL结尾的字符串数组
  int argc = 0;
  if (argv != NULL) {
    while (argv[argc]!=NULL) {
      int len = strlen(argv[argc]) + 1;
      string_area_len += len;
      top -= len;
      strcpy((char*)(top + 1), argv[argc]);
      argc++;
      // 刚开始想override，来暂存新的地址，但是因为是const，而且也不应该修改用户输入的参数
      // argv[argc] = (char*)(top + 1);
    }
  }

  int envc = 0;
  if (envp != NULL) {
    while (envp[envc]!=NULL) {
      int len = strlen(envp[envc]) + 1;
      string_area_len += len;
      top -= len;
      strcpy((char*)(top + 1), envp[envc]);
      envc++;
    }
  }
  *top = 0;

  uintptr_t* env_top = (uintptr_t*)top - envc;

  top = (char*)env_top;
  top--;
  *top = 0;

  uintptr_t* argc_top = (uintptr_t*)top - argc;

  *(argc_top - 1) = argc;
  pcb->cp->GPRx = (uintptr_t)(argc_top - 1);

  top = (char*)heap.end;
  argc = 0;
  if (argv != NULL) {
    while (argv[argc]!=NULL) {
      int len = strlen(argv[argc]) + 1;
      top -= len;
      argc_top[argc] = (uintptr_t)(top + 1);
      argc++;
    }
  }
  envc = 0;
  if (envp != NULL) {
    while (envp[envc]!=NULL) {
      int len = strlen(envp[envc]) + 1;
      top -= len;
      env_top[envc] = (uintptr_t)(top + 1);
      envc++;
    }
  }
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
