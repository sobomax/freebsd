/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2003 Alexander Kabaev.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RTLD_LOCK_H_
#define	_RTLD_LOCK_H_

#define	RTLI_VERSION_ONE	0x01
#define	RTLI_VERSION		0x02

#define	MAX_RTLD_LOCKS	8

struct _thr_codeptr {
       const char *fname;
       int linen;
       const char *funcn;
};

#define THR_HERE       (                                               \
       {static const struct _thr_codeptr _here = {.fname = __FILE__,   \
        .linen = __LINE__, .funcn = __func__}; &_here;})

/*
 * This structure is part of the ABI between rtld and threading
 * libraries, like libthr and even libc_r.  Its layout is fixed and
 * can be changed only by appending new fields at the end, with the
 * bump of RTLI_VERSION.
 */
struct RtldLockInfo
{
	/*
	 * Valid if the object calling _rtld_thread_init() exported
	 * symbol _pli_rtli_version.  Otherwise assume RTLI_VERSION_ONE.
	 */
	unsigned int rtli_version;

	void *(*lock_create)(void);
	void  (*lock_destroy)(void *);
	void  (*_rlock_acquire)(void *, const struct _thr_codeptr *);
	void  (*_wlock_acquire)(void *, const struct _thr_codeptr *);
	void  (*_lock_release)(void *, const struct _thr_codeptr *);
	int   (*thread_set_flag)(int);
	int   (*thread_clr_flag)(int);
	void  (*at_fork)(void);

	/* Version 2 fields */
	char *(*dlerror_loc)(void);
	int  *(*dlerror_seen)(void);
	struct pthread *(*curthread)(void);
	int   dlerror_loc_sz;
};

#if defined(IN_RTLD) || defined(PTHREAD_KERNEL)

void _rtld_thread_init(struct RtldLockInfo *) __exported;
void _rtld_atfork_pre(int *) __exported;
void _rtld_atfork_post(int *) __exported;

#endif /* IN_RTLD || PTHREAD_KERNEL */

#ifdef IN_RTLD

struct rtld_lock;
typedef struct rtld_lock *rtld_lock_t;

extern rtld_lock_t	rtld_bind_lock;
extern rtld_lock_t	rtld_libc_lock;
extern rtld_lock_t	rtld_phdr_lock;

extern struct RtldLockInfo lockinfo;

#define	RTLD_LOCK_UNLOCKED	0
#define	RTLD_LOCK_RLOCKED	1
#define	RTLD_LOCK_WLOCKED	2

struct Struct_RtldLockState;
typedef struct Struct_RtldLockState RtldLockState;

void	_rlock_acquire(rtld_lock_t, RtldLockState *, const struct _thr_codeptr *);
#define rlock_acquire(x, y) _rlock_acquire((x), (y), THR_HERE)
void 	_wlock_acquire(rtld_lock_t, RtldLockState *, const struct _thr_codeptr *);
#define wlock_acquire(x, y) _wlock_acquire((x), (y), THR_HERE)
void	_lock_release(rtld_lock_t, RtldLockState *, const struct _thr_codeptr *);
#define lock_release(x, y) _lock_release((x), (y), THR_HERE)
void	lock_upgrade(rtld_lock_t, RtldLockState *);
void	lock_restart_for_upgrade(RtldLockState *);
struct pthread *curthread(void);

void	dlerror_dflt_init(void);

#endif	/* IN_RTLD */

#endif
