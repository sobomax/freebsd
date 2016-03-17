#include <stdint.h>
#include <stdlib.h>

#include "mkuzip.h"
#include "mkuz_blk.h"

struct mkuz_blk *
mkuz_blk_ctor(size_t blen)
{
    struct mkuz_blk *rval;

    rval = mkuz_safe_zmalloc(sizeof(struct mkuz_blk) + blen);
    rval->alen = blen;
    rval->br_offset = OFFSET_UNDEF;
    return (rval);
}
