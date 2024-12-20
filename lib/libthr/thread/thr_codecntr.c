#include <stdio.h>
#include <stdlib.h>

#include "rtld_lock.h"
#include "thr_private.h"
#include "thr_codecntr.h"

#define CODEPTR_FMT(fmt, mlp, args...) ("%s+%d, %s()" fmt), (mlp)->fname, (mlp)->linen, (mlp)->funcn, ## args

void
_thr_codecntr_dump(struct _thr_codecntr *tccp)
{
	for (int i = 0; i < _THR_CH_LEN; i++) {
		const struct _thr_codeptr *lp = tccp[i].ptr;
		unsigned long cc = tccp[i].cnt;

		if (lp == NULL)
			break;
		_thread_printf(STDERR_FILENO, CODEPTR_FMT(":\t%lu\n", lp, cc));
	}
}
