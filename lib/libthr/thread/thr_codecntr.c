#include <stdio.h>
#include <stdlib.h>

#include "thr_private.h"
#include "thr_codecntr.h"

#define CODEPTR_FMT(fmt, mlp, args...) ("%s+%d, %s()" fmt), (mlp)->fname, (mlp)->linen, (mlp)->funcn, ## args

void
_thr_codecntr_dump(struct _thr_codecntr *tccp)
{
	for (int i = 0; i < _THR_CH_LEN; i++) {
		const struct _thr_codeptr *lp;
		unsigned long cc;

		lp = atomic_load(&tccp[i].ptr);
		if (lp == NULL)
			break;
		cc = atomic_load(&tccp[i].cnt);
		_thread_printf(STDERR_FILENO, CODEPTR_FMT(":\t%lu\n", lp, cc));
	}
}
