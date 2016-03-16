struct mkuz_blk_info {
    uint64_t offset;
    ssize_t len;
    uint32_t blkno;
};

struct mkuz_blk {
    struct mkuz_blk_info info;
    unsigned char data[];
};
