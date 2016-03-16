struct mkuz_wrk_itm {
    struct mkuz_blk *this;
    struct mkuz_blk *next;
};

struct mkuz_fifo_queue {
    pthread_mutex_t mtx;
    pthread_cond_t cvar;
    struct mkuz_wrk_itm *first;
    struct mkuz_wrk_itm *last;
};

#define MKUZ_WRK_ITM_EOF	(void *)0x1

struct mkuz_conveyer {
    /*
     * Work items are places in here, and picked up by workers in a FIFO
     * fashion.
     */
    struct mkuz_fifo_queue wrk_queue;
    /*
     * Results are dropped into this FIFO and consumer is buzzed to pick them
     * up
     */
    struct mkuz_fifo_queue results;
};

void mkuz_conveyer_init(struct mkuz_conveyer *);

void mkuz_fqueue_init(struct mkuz_fifo_queue *);
void mkuz_fqueue_enq(struct mkuz_fifo_queue *, struct mkuz_blk *);
struct mkuz_blk *mkuz_fqueue_deq(struct mkuz_fifo_queue *);
