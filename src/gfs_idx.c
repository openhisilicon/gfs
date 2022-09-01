/* gfs 索引文件操作 */
#include "gfs_idx.h"
#include "gfs_rec.h"
#include "printd.h"

int idx_hdr_read(int fd, gfs_hdr_t *hdr)
{
    pread(fd, hdr, GFS_IDX_HDR_SIZE, 0);
    return 0;
}
int idx_hdr_write(int fd, gfs_hdr_t *hdr)
{
    pwrite(fd, hdr, GFS_IDX_HDR_SIZE, 0);
    return 0;
}

int idx_bkt_seg_read(int fd, int bkt_no, gfs_bkt_t *bkt)
{
    int ret = 0;
    ret = pread(fd, bkt, GFS_IDX_BKT_SIZE, GFS_IDX_HDR_SIZE+bkt_no*GFS_IDX_BKT_SIZE);
    assert(ret == GFS_IDX_BKT_SIZE);
    //printf("idx_bkt_seg_read bkt_no:%d, seg_cnt:%d\n", bkt_no, bkt->seg_cnt);
    #if 0//def DEBUG
    if(!bkt->fpic && bkt->seg_cnt > 0)
    {
        gfs_vseg_t *vseg = (gfs_vseg_t*)(bkt+1);
        printf("idx_bkt_seg_read vseg[%d]: idx_end:%X, dat_end:%X\n"
            , bkt->seg_cnt-1
            , vseg[bkt->seg_cnt-1].idx_end
            , vseg[bkt->seg_cnt-1].dat_end);
    }
    #endif
    return 0;
}
int idx_bkt_write(int fd, int bkt_no, gfs_bkt_t *bkt)
{
    //printf("idx_bkt_write bkt_no:%d, seg_cnt:%d\n", bkt_no, bkt->seg_cnt);
    pwrite(fd, bkt, sizeof(gfs_bkt_t), GFS_IDX_HDR_SIZE+bkt_no*GFS_IDX_BKT_SIZE);
    return 0;
}
int idx_seg_write(int fd, int bkt_no, int seg_no, int fpic, void *seg)
{
    int size = (fpic)?sizeof(gfs_pseg_t):sizeof(gfs_vseg_t);
    #if 0//def DEBUG
    if(!fpic)
    {
        gfs_vseg_t *vseg = (gfs_vseg_t*)seg;
        printf("idx_seg_write bkt_no:%d, seg_no:%d, vseg: idx_end:%X, dat_end:%X\n"
                , bkt_no, seg_no, vseg->idx_end, vseg->dat_end);
    }
    #endif
    pwrite(fd, seg, size
            , GFS_IDX_HDR_SIZE+bkt_no*GFS_IDX_BKT_SIZE
              +sizeof(gfs_bkt_t)+seg_no*size);
    return 0;
}