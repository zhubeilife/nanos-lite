#include <common.h>
#include <sys/time.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

static int screen_w = 0, screen_h = 0;

int get_am_uptime(struct timeval *tv) {
  AM_TIMER_UPTIME_T uptime;
  uint32_t sec, us;
  uptime = io_read(AM_TIMER_UPTIME);
  tv->tv_sec = uptime.us / 1000000;
  tv->tv_usec = uptime.us % 1000000;
  return 0;
}

size_t serial_write(const void *buf, size_t offset, size_t len) {
  char *str = (char *)buf;
  for (int i = 0; i < len; i++) {
    putch(str[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE) {
    return 0;
  }
  // TODO: should use snprintf at klibs to limit the lens
  char *p = (char *)buf;
  return sprintf((char*)p, "%s %s", ev.keydown ? "kd" : "ku", keyname[ev.keycode]);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T t = io_read(AM_GPU_CONFIG);

  // TODO: should use snprintf at klibs to limit the lens
  screen_w = t.width;
  screen_h = t.height;
  size_t count = sprintf(buf, "WIDTH: %d \nHEIGHT: %d", t.width, t.height);
  return count;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  // AM_DEVREG(11, GPU_FBDRAW, WR, int x, y; void *pixels; int w, h; bool sync);
  // get x y from offset
  AM_GPU_CONFIG_T t = io_read(AM_GPU_CONFIG);

  offset = offset / 4;
  int w = len / 4;

  int y = offset / t.width;
  int x = offset - y * t.width;

  io_write(AM_GPU_FBDRAW, x, y, (void*)buf, w, 1, true);
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
