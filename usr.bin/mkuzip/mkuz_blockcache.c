/*
 * Copyright (c) 2016 Maxim Sobolev <sobomax@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <err.h>
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(MKUZ_DEBUG)
# include <assert.h>
# include <stdio.h>
#endif

#include "mkuz_blockcache.h"
#include "mkuz_blk.h"

struct mkuz_blkcache {
    struct mkuz_blk_info hit;
    unsigned char digest[16];
    struct mkuz_blkcache *next;
};

static struct mkuz_blkcache blkcache;

static int
verify_match(int fd, const struct mkuz_blk *cbp, struct mkuz_blkcache *bcep)
{
    void *vbuf;
    ssize_t rlen;
    int rval;

    rval = -1;
    vbuf = malloc(cbp->info.len);
    if (vbuf == NULL) {
        goto e0;
    }
    if (lseek(fd, bcep->hit.offset, SEEK_SET) < 0) {
        goto e1;
    }
    rlen = read(fd, vbuf, cbp->info.len);
    if (rlen < 0 || (unsigned)rlen != cbp->info.len) {
        goto e2;
    }
    rval = (memcmp(cbp->data, vbuf, cbp->info.len) == 0) ? 1 : 0;
e2:
    lseek(fd, cbp->info.offset, SEEK_SET);
e1:
    free(vbuf);
e0:
    return (rval);
}

#define I2J(x)	((intmax_t)(x))
#define U2J(x)	((uintmax_t)(x))

struct mkuz_blk_info *
mkuz_blkcache_regblock(int fd, const struct mkuz_blk *bp)
{
    struct mkuz_blkcache *bcep;
    MD5_CTX mcontext;
    unsigned char mdigest[16];
    int rval;

#if defined(MKUZ_DEBUG)
    assert((unsigned)lseek(fd, 0, SEEK_CUR) == bp->info.offset);
#endif
    MD5Init(&mcontext);
    MD5Update(&mcontext, bp->data, bp->info.len);
    MD5Final(mdigest, &mcontext);
    if (blkcache.hit.len == 0) {
        bcep = &blkcache;
    } else {
        for (bcep = &blkcache; bcep != NULL; bcep = bcep->next) {
            if (bcep->hit.len != bp->info.len)
                continue;
            if (memcmp(mdigest, bcep->digest, sizeof(mdigest)) == 0) {
                break;
            }
        }
        if (bcep != NULL) {
            rval = verify_match(fd, bp, bcep);
            if (rval == 1) {
#if defined(MKUZ_DEBUG)
                fprintf(stderr, "cache hit %jd, %jd, %jd, %jd\n",
                  I2J(bcep->hit.blkno), I2J(bcep->hit.offset),
                  I2J(bp->info.offset), I2J(bp->info.len));
#endif
                return (&bcep->hit);
            }
            if (rval == 0) {
#if defined(MKUZ_DEBUG)
                fprintf(stderr, "block MD5 collision, you should try lottery, "
                  "man!\n");
#endif
                return (NULL);
            }
            warn("verify_match");
            return (NULL);
        }
        bcep = malloc(sizeof(struct mkuz_blkcache));
        if (bcep == NULL)
            return (NULL);
        memset(bcep, '\0', sizeof(struct mkuz_blkcache));
        bcep->next = blkcache.next;
        blkcache.next = bcep;
    }
    memcpy(bcep->digest, mdigest, sizeof(mdigest));
    bcep->hit.offset = bp->info.offset;
    bcep->hit.len = bp->info.len;
    bcep->hit.blkno = bp->info.blkno;
    return (NULL);
}
