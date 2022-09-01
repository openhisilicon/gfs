#ifndef __gfs_idx_h__
#define __gfs_idx_h__

/* gfs 索引文件操作
 * qq: 25088970 maohw
 */
 
#include "gfs.h"

int idx_hdr_read(int fd, gfs_hdr_t *hdr);
int idx_hdr_write(int fd, gfs_hdr_t *hdr);

int idx_bkt_seg_read(int fd, int bkt_no, gfs_bkt_t *bkt);
int idx_bkt_write(int fd, int bkt_no, gfs_bkt_t *bkt);
int idx_seg_write(int fd, int bkt_no, int seg_no, int fpic, void* seg);

#endif //~__gfs_idx_h__