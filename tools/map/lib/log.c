/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "log.h"

static struct {
  void *udata;
  log_LockFn lock;
  FILE *fp;
  int level;
  int quiet;
} L;


static const char *level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


static void lock(void)   {
  if (L.lock) {
    L.lock(L.udata, 1);
  }
}


static void unlock(void) {
  if (L.lock) {
    L.lock(L.udata, 0);
  }
}


void log_set_udata(void *udata) {
  L.udata = udata;
}


void log_set_lock(log_LockFn fn) {
  L.lock = fn;
}


void log_set_fp(FILE *fp) {
  L.fp = fp;
}


void log_set_level(int level) {
  L.level = level;
}


void log_set_quiet(int enable) {
  L.quiet = enable ? 1 : 0;
}


void log_log(int level, const char *file, int line, const char *fmt, ...) {
  if (level < L.level) {
    return;
  }

  /* Acquire lock */
  lock();

  /* Get current time */
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  /* Log to stderr */
  if (!L.quiet) {
    va_list args;
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(
      stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
      buf, level_colors[level], level_names[level], file, line);
#else
    fprintf(stderr, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
#endif
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  /* Log to file */
  if (L.fp) {
    va_list args;
    char buf[32];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
    fprintf(L.fp, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
    va_start(args, fmt);
    vfprintf(L.fp, fmt, args);
    va_end(args);
    fprintf(L.fp, "\n");
    fflush(L.fp);
  }

  /* Release lock */
  unlock();
}

