#define OFFSET_UNDEF      UINT64_MAX

struct mkuz_blk_info {
    uint64_t offset;
    size_t len;
    uint32_t blkno;
};

#define MKUZ_BLK_EOF        (void *)0x1
#define MKUZ_BLK_MORE       (void *)0x2

struct mkuz_blk {
    struct mkuz_blk_info info;
    size_t alen;
    uint64_t br_offset;
    unsigned char data[];
};

struct mkuz_blk *mkuz_blk_ctor(size_t);
