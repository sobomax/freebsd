#include <pthread.h>
#include <strings.h>

#include "mkuz_conveyer.h"

void
mkuz_conveyer_init(struct mkuz_conveyer *cp)
{
    bzero(cp, sizeof(struct mkuz_conveyer));
    mkuz_fqueue_init(&cp->wrk_queue);
    mkuz_fqueue_init(&cp->results);
}

void
mkuz_fqueue_init(struct mkuz_fifo_queue *fqp)
{

    pthread_mutex_init(&fqp->mtx, NULL);
    pthread_cond_init(&fqp->cvar, NULL);
}
