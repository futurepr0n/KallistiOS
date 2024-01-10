/* KallistiOS ##version##

   newlib_kill.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis

*/

#include <kos/thread.h>
#include <sys/reent.h>
#include <errno.h>

int _kill_r(struct _reent *reent, int pid, int sig) {
    if(pid <= 0 || pid == KOS_PID) {
        /* NULL signals are simply ignored, while any other
           signal will terminate the application with the signal
           value as its return value.
         */
        if(sig)
            exit(sig);

        return 0;
    }

    reent->_errno = ESRCH;
    return -1;
}
