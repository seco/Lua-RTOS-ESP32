/*
 * Lua RTOS, file descriptor implementation
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)sys_generic.c   8.9 (Berkeley) 2/14/95
 */

#include "syscalls.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <stdio.h>

#include <sys/panic.h>
#include <sys/status.h>
#include <sys/mutex.h>

struct mtx fd_mtx;

struct  filedesc *p_fd;     // File descriptor structure
static int nfiles = 0;      // Current number of open files
struct filelist filehead;   // Head of list of open files


// Do some initializations for sys calls
//   * initialization of file descriptor structure
//   * creation of file drscriptor mutex
//
// This function must be called before any syscall invocation, and it's called
// during the Lua RTOS booting process.
void _syscalls_init() {
	nfiles = 0;

    // Allocate space for file descriptor structure
    p_fd = (struct filedesc *)calloc(1, sizeof(struct filedesc));
    if (!p_fd) {
        panic("Cannot allocate space for file descriptors");
    }

    p_fd->fd_ofiles = (struct  file **)calloc(1, sizeof(struct  file *) * NDFILE);
    if (!p_fd->fd_ofiles) {
        panic("Cannot allocate space for open files");
    }

    p_fd->fd_ofileflags = (char *)calloc(1, sizeof(char) * NDFILE);
    if (!p_fd->fd_ofileflags) {
        panic("Cannot allocate space for open file flags");
    }

    p_fd->fd_nfiles = NDFILE;

    // Create file descriptor mutex
    mtx_init(&fd_mtx, NULL, NULL, 0);

	status_set(STATUS_SYSCALLS_INITED);
}

// Get number of open files
static int get_nfiles() {
    int files;

    mtx_lock(&fd_mtx);
    files = nfiles;
    mtx_unlock(&fd_mtx);

    return files;
}

// Increment number of open files
static void inc_nfiles() {
    mtx_lock(&fd_mtx);
    nfiles++;
    mtx_unlock(&fd_mtx);
}

// Decrement number of open files
static void dec_nfiles() {
    mtx_lock(&fd_mtx);
    nfiles--;
    mtx_unlock(&fd_mtx);
}

// Allocate a file descriptor
static int fdalloc(int want, int *result) {
    register struct filedesc *fdp = p_fd;
    register int i;
    int lim, last;

    mtx_lock(&fd_mtx);

    /*
     * Search for a free descriptor starting at the higher
     * of want or fd_freefile.  If that fails, consider
     * expanding the ofile array.
     */
    lim = NDFILE;
    for (;;) {
        last = min(fdp->fd_nfiles, lim);
        if ((i = want) < fdp->fd_freefile)
            i = fdp->fd_freefile;
        for (; i < last; i++) {
            if (fdp->fd_ofiles[i] == NULL) {
                fdp->fd_ofileflags[i] = 0;
                if (i > fdp->fd_lastfile)
                    fdp->fd_lastfile = i;
                if (want <= fdp->fd_freefile)
                    fdp->fd_freefile = i;
                *result = i;
                mtx_unlock(&fd_mtx);

                return (0);
            }
        }

        mtx_unlock(&fd_mtx);

        break;
    }

    return (EMFILE);
}

/*
 * Free a file descriptor.
 */
static void ffree(register struct file *fp) {
    mtx_lock(&fd_mtx);
    LIST_REMOVE(fp, f_list);
    mtx_unlock(&fd_mtx);

    dec_nfiles();

    if (fp->f_data) {
        FREE(fp->f_data);
    }

    if (fp->f_dir) {
        FREE(fp->f_dir);
   }

    if (fp->f_fs) {
    	FREE(fp->f_fs);
    }

    if (fp->f_path) {
    	FREE(fp->f_path);
    }

    FREE(fp);
}

/*
 * Common code for dup, dup2, and fcntl(F_DUPFD).
 */
static int finishdup(fdp, old, new, retval)
    register struct filedesc *fdp;
    register int old, new;
    register int *retval;
{
    register struct file *fp;

    mtx_lock(&fd_mtx);

    fp = fdp->fd_ofiles[old];
    fdp->fd_ofiles[new] = fp;
    fdp->fd_ofileflags[new] = fdp->fd_ofileflags[old] &~ UF_EXCLOSE;
    fp->f_count++;
    if (new > fdp->fd_lastfile)
        fdp->fd_lastfile = new;
    *retval = new;

    mtx_unlock(&fd_mtx);

    return (0);
}

static void munmapfd(int fd) {
    p_fd->fd_ofileflags[fd] &= ~UF_MAPPED;
}


/*
 * Create a new open file structure and allocate
 * a file decriptor for the process that refers to it.
 */
int falloc(struct file **resultfp, int *resultfd) {
    register struct file *fp, *fq;
    int error, i;

    error = fdalloc(0, &i);
    if (error)
        return (error);
    if (get_nfiles() >= NDFILE) {
        return (ENFILE);
    }

    /*
     * Allocate a new file descriptor.
     * If the process has file descriptor zero open, add to the list
     * of open files at that point, otherwise put it at the front of
     * the list of open files.
     */
    inc_nfiles();
    MALLOC(fp, struct file *, sizeof(struct file));
    bzero(fp, sizeof(struct file));

    mtx_lock(&fd_mtx);

    fq = p_fd->fd_ofiles[0];
    if (fq) {
        LIST_INSERT_AFTER(fq, fp, f_list);
    } else {
        LIST_INSERT_HEAD(&filehead, fp, f_list);
    }

    p_fd->fd_ofiles[i] = fp;

    mtx_unlock(&fd_mtx);

    fp->f_count = 1;

    if (resultfp)
        *resultfp = fp;
    if (resultfd)
        *resultfd = i;

    return (0);
}

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 * Note: p may be NULL when closing a file
 * that was being passed in a message.
 */
int closef(register struct file *fp) {
    register struct filedesc *fdp = p_fd;
    register u_char *pf;
    int error = 0;

    if (fp == NULL)
        return (0);

    mtx_lock(&fd_mtx);

    if ((u_int)fp->f_fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fp->f_fd]) == NULL) {
        mtx_unlock(&fd_mtx);
        errno = EBADF;
        return -1;
    }

    pf = (u_char *)&fdp->fd_ofileflags[fp->f_fd];
    if (*pf & UF_MAPPED)
        (void) munmapfd(fp->f_fd);

    fdp->fd_ofiles[fp->f_fd] = NULL;

    while (fdp->fd_lastfile > 0 && fdp->fd_ofiles[fdp->fd_lastfile] == NULL)
        fdp->fd_lastfile--;

    if (fp->f_fd < fdp->fd_freefile)
        fdp->fd_freefile = fp->f_fd;

    *pf = 0;

    mtx_unlock(&fd_mtx);

    ffree(fp);

    return (error);
}

// Get a file from it's file descriptor
struct file *get_file(int fd) {
    register struct filedesc *fdp = p_fd;
    register struct file *fp;

    fd = fd & 0b111111111111;

    mtx_lock(&fd_mtx);
    if ((u_int)fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL) {
        mtx_unlock(&fd_mtx);
        return NULL;
    }

    mtx_unlock(&fd_mtx);
    return fp;
}

/*
 * Duplicate a file descriptor.
 */
/* ARGSUSED */
int dup(int oldfd) {
    register struct filedesc *fdp = p_fd;
    u_int old;
    int new, error, retval;

    old = oldfd;

    mtx_lock(&fd_mtx);
    if (old >= fdp->fd_nfiles || fdp->fd_ofiles[old] == NULL) {
        mtx_unlock(&fd_mtx);

        errno = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);

    error = fdalloc(0, &new);
    if (error) {
        errno = error;
        return -1;
    }

    finishdup(fdp, (int)old, new, &retval);

    return retval;
}

/*
 * Duplicate a file descriptor to a particular value.
 */
int dup2(int oldfd, int newfd) {
    register struct filedesc *fdp = p_fd;
    register int old = oldfd, new = newfd;
    int i, error, retval;

    mtx_lock(&fd_mtx);
    if (old >= fdp->fd_nfiles ||
        fdp->fd_ofiles[old] == NULL ||
        new >= NDFILE) {
        mtx_unlock(&fd_mtx);
        errno = EBADF;
        return -1;

    }
    mtx_unlock(&fd_mtx);

    if (old == new) {
        return new;
    }

    mtx_lock(&fd_mtx);
    if (new >= fdp->fd_nfiles) {
        mtx_unlock(&fd_mtx);
        error = fdalloc(new, &i);
        if (error) {
			mtx_unlock(&fd_mtx);
            errno = error;
            return -1;
        }
        if (new != i)
            panic("dup2: fdalloc");
    } else if (fdp->fd_ofiles[new]) {
        if (fdp->fd_ofileflags[new] & UF_MAPPED)
            (void) munmapfd(new);

        mtx_unlock(&fd_mtx);
        /*
         * dup2() must succeed even if the close has an error.
         */
        (void) closef(fdp->fd_ofiles[new]);
    }

    finishdup(fdp, (int)old, (int)new, &retval);

    return retval;
}

int fcntl(int fd, int cmd, ... ) {
    register struct filedesc *fdp = p_fd;
    register struct file *fp;
    register char *pop;
    int i, error;

    int retval;
    va_list valist;

    mtx_lock(&fd_mtx);
    if ((u_int)fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL) {
        mtx_unlock(&fd_mtx);
        errno = EBADF;
        return -1;
    }

    pop = &fdp->fd_ofileflags[fd];

    mtx_unlock(&fd_mtx);

    switch (cmd) {
    case F_DUPFD:
        va_start(valist, cmd);
        int arg1 = va_arg(valist, int);
        va_end(valist);

        if (arg1 >= NDFILE) {
            errno = EINVAL;
            return -1;
        }
        error = fdalloc(arg1, &i);
        if (error) {
            errno = error;
            return -1;
        }
        finishdup(fdp, fd, i, &retval);

        return retval;

    case F_GETFD:
        retval = *pop & 1;
        return retval;

      case F_SETFD:
        va_start(valist, cmd);
        long arg2 = va_arg(valist, long);
        va_end(valist);

        *pop = (*pop &~ 1) | (arg2 & 1);
        return (0);

    case F_GETFL:
        retval = OFLAGS(fp->f_flag);
        return retval;

    case F_SETFL:
        va_start(valist, cmd);
        long arg3 = va_arg(valist, long);
        va_end(valist);

        fp->f_flag &= ~FCNTLFLAGS;
        fp->f_flag |= FFLAGS(arg3) & FCNTLFLAGS;
        //tmp = fp->f_flag & FNONBLOCK;
        // TO DO
        //error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&tmp);
        //if (error) {
        //    errno = error;
        //    return -1;
        //}

        return 0;

/*
        tmp = fp->f_flag & FASYNC;
        error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, (caddr_t)&tmp);
        if (!error)
            return (0);

        fp->f_flag &= ~FNONBLOCK;
        tmp = 0;
        (void) (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&tmp);

        errno = error;
        return -1;
*/

/*
    case F_GETOWN:
        if (fp->f_type == DTYPE_SOCKET) {
            *retval = ((struct socket *)fp->f_data)->so_pgid;
            return (0);
        }
        error = (*fp->f_ops->fo_ioctl)
            (fp, TIOCGPGRP, (caddr_t)retval, p);

        if (error) {
            errno = error;
            return -1;
        }

        retval = -retval;
        return (retval);

    case F_SETOWN:
        if (fp->f_type == DTYPE_SOCKET) {
            ((struct socket *)fp->f_data)->so_pgid =
                (long)arg;
            return (0);
        }
        if ((long)SCARG(uap, arg) <= 0) {
            SCARG(uap, arg) = (void *)(-(long)SCARG(uap, arg));
        } else {
            struct proc *p1 = pfind((long)SCARG(uap, arg));
            if (p1 == 0)
                return (ESRCH);
            SCARG(uap, arg) = (void *)(long)p1->p_pgrp->pg_id;
        }
        return ((*fp->f_ops->fo_ioctl)
            (fp, TIOCSPGRP, (caddr_t)&SCARG(uap, arg), p));
*/

/*
    case F_SETLKW:
        flg |= F_WAIT;
*/
        /* Fall into F_SETLK */

/*
    case F_SETLK:
        if (fp->f_type != DTYPE_VNODE)
            return (EBADF);
        vp = (struct vnode *)fp->f_data;
*/
        /* Copy in the lock structure */
/*
        error = copyin((caddr_t)SCARG(uap, arg), (caddr_t)&fl,
            sizeof (fl));
        if (error)
            return (error);
        if (fl.l_whence == SEEK_CUR)
            fl.l_start += fp->f_offset;
        switch (fl.l_type) {

        case F_RDLCK:
            if ((fp->f_flag & FREAD) == 0)
                return (EBADF);
            p->p_flag |= P_ADVLOCK;
            return (VOP_ADVLOCK(vp, (caddr_t)p, F_SETLK, &fl, flg));

        case F_WRLCK:
            if ((fp->f_flag & FWRITE) == 0)
                return (EBADF);
            p->p_flag |= P_ADVLOCK;
            return (VOP_ADVLOCK(vp, (caddr_t)p, F_SETLK, &fl, flg));

        case F_UNLCK:
            return (VOP_ADVLOCK(vp, (caddr_t)p, F_UNLCK, &fl,
                F_POSIX));

        default:
            return (EINVAL);
        }

    case F_GETLK:
        if (fp->f_type != DTYPE_VNODE)
            return (EBADF);
        vp = (struct vnode *)fp->f_data;*/
        /* Copy in the lock structure */
/*        error = copyin((caddr_t)SCARG(uap, arg), (caddr_t)&fl,
            sizeof (fl));
        if (error)
            return (error);
        if (fl.l_whence == SEEK_CUR)
            fl.l_start += fp->f_offset;
        error = VOP_ADVLOCK(vp, (caddr_t)p, F_GETLK, &fl, F_POSIX);
        if (error)
            return (error);
        return (copyout((caddr_t)&fl, (caddr_t)SCARG(uap, arg),
            sizeof (fl)));
*/

    default:
        return (EINVAL);
    }
    /* NOTREACHED */
}
