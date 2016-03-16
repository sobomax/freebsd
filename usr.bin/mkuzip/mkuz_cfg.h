struct mkuz_cfg {
    int fdr;
    int fdw;
    int verbose;
    int no_zcomp;
    int en_dedup;
    const struct mkuz_format *handler;
};
