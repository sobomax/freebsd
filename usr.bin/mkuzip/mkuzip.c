/*
 * Copyright (c) 2004-2016 Maxim Sobolev <sobomax@FreeBSD.org>
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
#include <sys/disk.h>
#include <sys/endian.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mkuzip.h"
#include "mkuz_cloop.h"
#include "mkuz_blockcache.h"
#include "mkuz_zlib.h"
#include "mkuz_lzma.h"
#include "mkuz_blk.h"
#include "mkuz_cfg.h"
#include "mkuz_conveyer.h"

#define DEFINE_RAW_METHOD(func, rval, args...) typedef rval (*func##_t)(args)

#define DEFAULT_CLSTSIZE	16384

DEFINE_RAW_METHOD(f_init, void *, uint32_t);
DEFINE_RAW_METHOD(f_compress, struct mkuz_blk *, void *, const struct mkuz_blk *);

struct mkuz_format {
	const char *magic;
	const char *default_sufx;
	f_init_t f_init;
	f_compress_t f_compress;
};

static struct mkuz_format uzip_fmt = {
	.magic = CLOOP_MAGIC_ZLIB,
	.default_sufx = DEFAULT_SUFX_ZLIB,
	.f_init = &mkuz_zlib_init,
	.f_compress = &mkuz_zlib_compress
};

static struct mkuz_format ulzma_fmt = {
        .magic = CLOOP_MAGIC_LZMA,
        .default_sufx = DEFAULT_SUFX_LZMA,
        .f_init = &mkuz_lzma_init,
        .f_compress = &mkuz_lzma_compress
};

static struct mkuz_blk *readblock(int, u_int32_t);
static void usage(void);
static void cleanup(void);
static int  memvcmp(const void *, unsigned char, size_t);

static char *cleanfile = NULL;

static struct mkuz_blk *
mkuz_blk_queue(struct mkuz_cfg *cfp, void *c_ctx, uint64_t offset,
    const struct mkuz_blk *qbp)
{
	struct mkuz_blk_info *chit;
	struct mkuz_blk *oblk;

	oblk = cfp->handler->f_compress(c_ctx, qbp);
	oblk->info.offset = offset;
	if (cfp->en_dedup != 0) {
		chit = mkuz_blkcache_regblock(cfp->fdw, oblk);
		/*
		 * There should be at least one non-empty block
		 * between us and the backref'ed offset, otherwise
		 * we won't be able to parse that sequence correctly
		 * as it would be indistinguishible from another
		 * empty block.
		 */
		if (chit != NULL) {
			oblk->br_offset = chit->offset;
		}
	}
	return (oblk);
}

int main(int argc, char **argv)
{
	struct mkuz_cfg cfs;
	char *iname, *oname;
	uint64_t *toc;
	int i, opt, tmp;
	struct {
		int en;
		FILE *f;
	} summary;
	struct iovec iov[2];
	struct stat sb;
	uint64_t offset, last_offset;
	struct cloop_header hdr;
	struct mkuz_conveyer cbelt;
	int nworkers;
        void *c_ctx;

	memset(&hdr, 0, sizeof(hdr));
	mkuz_conveyer_init(&cbelt);
	hdr.blksz = DEFAULT_CLSTSIZE;
	oname = NULL;
	cfs.verbose = 0;
	cfs.no_zcomp = 0;
	cfs.en_dedup = 0;
	summary.en = 0;
	summary.f = stderr;
	cfs.handler = &uzip_fmt;
        nworkers = 1;
	struct mkuz_blk *iblk, *oblk;

	while((opt = getopt(argc, argv, "o:s:vZdLS")) != -1) {
		switch(opt) {
		case 'o':
			oname = optarg;
			break;

		case 's':
			tmp = atoi(optarg);
			if (tmp <= 0) {
				errx(1, "invalid cluster size specified: %s",
				    optarg);
				/* Not reached */
			}
			hdr.blksz = tmp;
			break;

		case 'v':
			cfs.verbose = 1;
			break;

		case 'Z':
			cfs.no_zcomp = 1;
			break;

		case 'd':
			cfs.en_dedup = 1;
			break;

		case 'L':
			cfs.handler = &ulzma_fmt;
			break;

		case 'S':
			summary.en = 1;
			summary.f = stdout;
			break;

		default:
			usage();
			/* Not reached */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
		/* Not reached */
	}

	strcpy(hdr.magic, cfs.handler->magic);

	if (cfs.en_dedup != 0) {
		hdr.magic[CLOOP_OFS_VERSN] = CLOOP_MAJVER_3;
		hdr.magic[CLOOP_OFS_COMPR] =
		    tolower(hdr.magic[CLOOP_OFS_COMPR]);
	}

	c_ctx = cfs.handler->f_init(hdr.blksz);

	iname = argv[0];
	if (oname == NULL) {
		asprintf(&oname, "%s%s", iname, cfs.handler->default_sufx);
		if (oname == NULL) {
			err(1, "can't allocate memory");
			/* Not reached */
		}
	}

	signal(SIGHUP, exit);
	signal(SIGINT, exit);
	signal(SIGTERM, exit);
	signal(SIGXCPU, exit);
	signal(SIGXFSZ, exit);
	atexit(cleanup);

	cfs.fdr = open(iname, O_RDONLY);
	if (cfs.fdr < 0) {
		err(1, "open(%s)", iname);
		/* Not reached */
	}
	if (fstat(cfs.fdr, &sb) != 0) {
		err(1, "fstat(%s)", iname);
		/* Not reached */
	}
	if (S_ISCHR(sb.st_mode)) {
		off_t ms;

		if (ioctl(cfs.fdr, DIOCGMEDIASIZE, &ms) < 0) {
			err(1, "ioctl(DIOCGMEDIASIZE)");
			/* Not reached */
		}
		sb.st_size = ms;
	} else if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "%s: not a character device or regular file\n",
			iname);
		exit(1);
	}
	hdr.nblocks = sb.st_size / hdr.blksz;
	if ((sb.st_size % hdr.blksz) != 0) {
		if (cfs.verbose != 0)
			fprintf(stderr, "file size is not multiple "
			"of %d, padding data\n", hdr.blksz);
		hdr.nblocks++;
	}
	toc = mkuz_safe_malloc((hdr.nblocks + 1) * sizeof(*toc));

	cfs.fdw = open(oname, (cfs.en_dedup ? O_RDWR : O_WRONLY) | O_TRUNC | O_CREAT,
		   S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	if (cfs.fdw < 0) {
		err(1, "open(%s)", oname);
		/* Not reached */
	}
	cleanfile = oname;

	/* Prepare header that we will write later when we have index ready. */
	iov[0].iov_base = (char *)&hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = (char *)toc;
	iov[1].iov_len = (hdr.nblocks + 1) * sizeof(*toc);
	offset = iov[0].iov_len + iov[1].iov_len;

	/* Reserve space for header */
	lseek(cfs.fdw, offset, SEEK_SET);

	if (cfs.verbose != 0)
		fprintf(stderr, "data size %ju bytes, number of clusters "
		    "%u, index length %zu bytes\n", sb.st_size,
		    hdr.nblocks, iov[1].iov_len);

	last_offset = 0;
        iblk = oblk = NULL;
	for(i = 0; iblk != MKUZ_BLK_EOF; i++) {
getmore:
		iblk = readblock(cfs.fdr, hdr.blksz);
		if (iblk != MKUZ_BLK_EOF) {
			if (cfs.no_zcomp == 0 &&
			    memvcmp(iblk->data, '\0', iblk->info.len) != 0) {
				/* All zeroes block */
				oblk = mkuz_blk_ctor(0);
			} else {
				oblk = mkuz_blk_queue(&cfs, c_ctx, offset, iblk);
				if (oblk == MKUZ_BLK_MORE) {
                                	goto getmore;
				}
				oblk->info.blkno = i;

			}
		} else {
			oblk = mkuz_blk_ctor(DEV_BSIZE - (offset % DEV_BSIZE));
			oblk->info.len = oblk->alen;
			if (cfs.verbose != 0)
				fprintf(stderr, "padding data with %lu bytes "
				    "so that file size is multiple of %d\n",
				    (u_long)oblk->alen, DEV_BSIZE);
		}
		if (oblk->br_offset != OFFSET_UNDEF && oblk->br_offset != last_offset) {
			toc[i] = htobe64(oblk->br_offset);
			oblk->info.len = 0;
		} else {
			if (oblk->info.len > 0 && write(cfs.fdw, oblk->data,
			    oblk->info.len) < 0) {
				err(1, "write(%s)", oname);
				/* Not reached */
			}
			toc[i] = htobe64(offset);
			last_offset = offset;
			offset += oblk->info.len;
		}
		if (iblk != MKUZ_BLK_EOF && cfs.verbose != 0) {
			fprintf(stderr, "cluster #%d, in %u bytes, "
			    "out len=%lu offset=%lu", i, hdr.blksz,
			    (u_long)oblk->info.len, (u_long)be64toh(toc[i]));
#if 0
			if (chit != NULL) {
				fprintf(stderr, " (backref'ed to #%d)",
				    chit->blkno);
			}
#endif
			fprintf(stderr, "\n");
			free(iblk);
		}
		free(oblk);
	}
	close(cfs.fdr);

	if (cfs.verbose != 0 || summary.en != 0)
		fprintf(summary.f, "compressed data to %ju bytes, saved %lld "
		    "bytes, %.2f%% decrease.\n", offset,
		    (long long)(sb.st_size - offset),
		    100.0 * (long long)(sb.st_size - offset) /
		    (float)sb.st_size);

	/* Convert to big endian */
	hdr.blksz = htonl(hdr.blksz);
	hdr.nblocks = htonl(hdr.nblocks);
	/* Write headers into pre-allocated space */
	lseek(cfs.fdw, 0, SEEK_SET);
	if (writev(cfs.fdw, iov, 2) < 0) {
		err(1, "writev(%s)", oname);
		/* Not reached */
	}
	cleanfile = NULL;
	close(cfs.fdw);

	exit(0);
}

static struct mkuz_blk *
readblock(int fd, u_int32_t clstsize)
{
	int numread;
	struct mkuz_blk *rval;
	static int blockcnt;
	off_t cpos;

	rval = mkuz_blk_ctor(clstsize);

	rval->info.blkno = blockcnt;
	blockcnt += 1;
	cpos = lseek(fd, 0, SEEK_CUR);
	if (cpos < 0) {
		err(1, "readblock: lseek() failed");
		/* Not reached */
	}
	rval->info.offset = cpos;

	numread = read(fd, rval->data, clstsize);
	if (numread < 0) {
		err(1, "readblock: read() failed");
		/* Not reached */
	}
	if (numread == 0) {
		free(rval);
		return MKUZ_BLK_EOF;
	}
	rval->info.len = numread;
	return rval;
}

static void
usage(void)
{

	fprintf(stderr, "usage: mkuzip [-vZdLS] [-o outfile] [-s cluster_size] "
	    "infile\n");
	exit(1);
}

void *
mkuz_safe_malloc(size_t size)
{
	void *retval;

	retval = malloc(size);
	if (retval == NULL) {
		err(1, "can't allocate memory");
		/* Not reached */
	}
	return retval;
}

void *
mkuz_safe_zmalloc(size_t size)
{
	void *retval;

	retval = mkuz_safe_malloc(size);
	bzero(retval, size);
	return retval;
}

static void
cleanup(void)
{

	if (cleanfile != NULL)
		unlink(cleanfile);
}

static int
memvcmp(const void *memory, unsigned char val, size_t size)
{
    const u_char *mm;

    mm = (const u_char *)memory;
    return (*mm == val) && memcmp(mm, mm + 1, size - 1) == 0;
}
