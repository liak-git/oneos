/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        libc_signal.h
 *
 * @brief       Supplement to the standard C library file.
 *
 * @revision
 * Date         Author          Notes
 * 2020-04-17   OneOS team      First Version
 ***********************************************************************************************************************
 */
#ifndef __LIBC_SIGNAL_H__
#define __LIBC_SIGNAL_H__

#include <oneos_config.h>
#include <os_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CCONFIG_H
#include <cconfig.h>
#endif

#ifndef HAVE_SIGVAL
/*  Signal Generation and Delivery, P1003.1b-1993, p. 63
    NOTE: P1003.1c/D10, p. 34 adds sigev_notify_function and
    sigev_notify_attributes to the sigevent structure.  */

union sigval 
{
    int    sival_int;    /* Integer signal value. */
    void  *sival_ptr;    /* Pointer signal value. */
};
#endif

#ifndef HAVE_SIGEVENT
struct sigevent
{
    int          sigev_notify;               /* Notification type. */
    int          sigev_signo;                /* Signal number. */
    union sigval sigev_value;                /* Signal value. */
    void         (*sigev_notify_function)( union sigval ); /* Notification function. */
    void         *sigev_notify_attributes;   /* Notification Attributes, really pthread_attr_t. */
};
#endif

#ifndef HAVE_SIGINFO
struct siginfo
{
    os_uint16_t si_signo;
    os_uint16_t si_code;
    union sigval si_value;
};
typedef struct siginfo siginfo_t;
#endif

#define SI_USER     0x01    /* Signal sent by kill(). */
#define SI_QUEUE    0x02    /* Signal sent by sigqueue(). */
#define SI_TIMER    0x03    /* Signal generated by expiration of a timer set by timer_settime(). */
#define SI_ASYNCIO  0x04    /* Signal generated by completion of an asynchronous I/O request. */
#define SI_MESGQ    0x05    /* Signal generated by arrival of a message on an empty message queue. */

#ifdef OS_USING_NEWLIB
#include <sys/signal.h>
#endif

#if defined(__CC_ARM) || defined(__CLANG_ARM)
#include <signal.h>
typedef unsigned long sigset_t;

#define SIGHUP       1
/* #define SIGINT       2 */
#define SIGQUIT      3
/* #define SIGILL       4 */
#define SIGTRAP      5
/* #define SIGABRT      6 */
#define SIGEMT       7
/* #define SIGFPE       8 */
#define SIGKILL      9
#define SIGBUS      10
/* #define SIGSEGV     11 */
#define SIGSYS      12
#define SIGPIPE     13
#define SIGALRM     14
/* #define SIGTERM     15 */
#define SIGURG      16
#define SIGSTOP     17
#define SIGTSTP     18
#define SIGCONT     19
#define SIGCHLD     20
#define SIGTTIN     21
#define SIGTTOU     22
#define SIGPOLL     23
#define SIGWINCH    24
/* #define SIGUSR1     25 */
/* #define SIGUSR2     26 */
#define SIGRTMIN    27
#define SIGRTMAX    31
#define NSIG        32

#define SIG_SETMASK 0   /* Set mask with sigprocmask(). */
#define SIG_BLOCK   1   /* Set of signals to block. */
#define SIG_UNBLOCK 2   /* Set of signals to, well, unblock. */

typedef void (*_sig_func_ptr)(int);

struct sigaction 
{
    _sig_func_ptr sa_handler;
    sigset_t      sa_mask;
    int           sa_flags;
};

#define sigaddset(what,sig) (*(what) |= (1 << (sig)), 0)
#define sigdelset(what,sig) (*(what) &= ~(1 << (sig)), 0)
#define sigemptyset(what)   (*(what) = 0, 0)
#define sigfillset(what)    (*(what) = ~(0), 0)
#define sigismember(what,sig) (((*(what)) & (1 << (sig))) != 0)

int sigprocmask (int how, const sigset_t *set, sigset_t *oset);
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

#elif defined(__IAR_SYSTEMS_ICC__)
#include <signal.h>
typedef unsigned long sigset_t;

#define SIGHUP       1
#define SIGINT       2
#define SIGQUIT      3
#define SIGILL       4
#define SIGTRAP      5
/* #define SIGABRT      6 */
#define SIGEMT       7
#define SIGFPE       8
#define SIGKILL      9
#define SIGBUS      10
#define SIGSEGV     11
#define SIGSYS      12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGURG      16
#define SIGSTOP     17
#define SIGTSTP     18
#define SIGCONT     19
#define SIGCHLD     20
#define SIGTTIN     21
#define SIGTTOU     22
#define SIGPOLL     23
#define SIGWINCH    24
#define SIGUSR1     25
#define SIGUSR2     26
#define SIGRTMIN    27
#define SIGRTMAX    31
#define NSIG        32

#define SIG_SETMASK 0   /* Set mask with sigprocmask(). */
#define SIG_BLOCK   1   /* Set of signals to block. */
#define SIG_UNBLOCK 2   /* Set of signals to, well, unblock. */

typedef void (*_sig_func_ptr)(int);

struct sigaction 
{
    _sig_func_ptr sa_handler;
    sigset_t      sa_mask;
    int           sa_flags;
};

#define sigaddset(what,sig) (*(what) |= (1 << (sig)), 0)
#define sigdelset(what,sig) (*(what) &= ~(1 << (sig)), 0)
#define sigemptyset(what)   (*(what) = 0, 0)
#define sigfillset(what)    (*(what) = ~(0), 0)
#define sigismember(what,sig) (((*(what)) & (1 << (sig))) != 0)

int sigprocmask (int how, const sigset_t *set, sigset_t *oset);
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
#endif

#ifdef __cplusplus
}
#endif

#endif

