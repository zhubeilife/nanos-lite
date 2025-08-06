#include <common.h>

void do_syscall(Context *c);
Context* schedule(Context *prev);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: {
      // Log("Nano lite: EVENT_YIELD\n");
      c = schedule(c);
      break;
    }
    case EVENT_SYSCALL: {
      // Log("Nano lite: EVENT_SYSCALL\n");
      do_syscall(c);
      break;
    }
    case EVENT_PAGEFAULT: printf("EVENT_PAGEFAULT\n"); break;
    case EVENT_ERROR: printf("EVENT_ERROR\n"); break;
    case EVENT_IRQ_TIMER: printf("EVENT_IRQ_TIMER\n"); break;
    case EVENT_IRQ_IODEV: printf("EVENT_IRQ_IODEV\n"); break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
