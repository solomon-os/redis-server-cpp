// sys/epoll.h — declaration-only stub for editor / IntelliSense use on macOS.
//
// PURPOSE: make clangd or VS Code IntelliSense resolve epoll_* functions,
// constants, and types while you EDIT on a Mac. It contains NO implementation
// and is never meant to be compiled into a running binary. You build and run in
// Docker/Lima, where the real glibc <sys/epoll.h> is picked up automatically.
//
// Values and signatures mirror Linux (glibc) exactly, so hover info and any
// constant evaluation your editor does are correct.
//
// The #error guard below means that if this file ever lands on a Linux include
// path (e.g. it leaked into the container build), the build fails loudly
// instead of silently shadowing the real header.
#pragma once

#if defined(__linux__)
#error                                                                         \
    "Editor-only epoll stub is on a Linux include path. Remove it from the build's -I flags and use the real <sys/epoll.h>."
#endif

#include <signal.h> /* sigset_t, for epoll_pwait */
#include <stdint.h>
#include <time.h> /* struct timespec, for epoll_pwait2 */

#ifdef __cplusplus
extern "C" {
#endif

/* Event flags — values match <linux/eventpoll.h>. */
#define EPOLLIN 0x00000001
#define EPOLLPRI 0x00000002
#define EPOLLOUT 0x00000004
#define EPOLLERR 0x00000008
#define EPOLLHUP 0x00000010
#define EPOLLRDNORM 0x00000040
#define EPOLLRDBAND 0x00000080
#define EPOLLWRNORM 0x00000100
#define EPOLLWRBAND 0x00000200
#define EPOLLMSG 0x00000400
#define EPOLLRDHUP 0x00002000
#define EPOLLEXCLUSIVE (1u << 28)
#define EPOLLWAKEUP (1u << 29)
#define EPOLLONESHOT (1u << 30)
#define EPOLLET (1u << 31)

/* epoll_ctl opcodes. */
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

/* Flag for epoll_create1 (== O_CLOEXEC). */
#define EPOLL_CLOEXEC 02000000

typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event {
  uint32_t events;   /* epoll event bitmask, EPOLL* flags above */
  epoll_data_t data; /* opaque user data, handed back by epoll_wait */
};
/* On real x86-64 Linux this struct is packed (12 bytes). Irrelevant here:
   nothing in this file is ever compiled into a binary. */

int epoll_create(int size);
int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents,
               int timeout);
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
                int timeout, const sigset_t *sigmask);
int epoll_pwait2(int epfd, struct epoll_event *events, int maxevents,
                 const struct timespec *timeout, const sigset_t *sigmask);

#ifdef __cplusplus
} /* extern "C" */
#endif
