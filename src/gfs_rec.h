#ifndef __gfs_rec_h__
#define __gfs_rec_h__

/* gfs Â¼Ïñ²Ù×÷ 
 * qq: 25088970 maohw
 */

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gfs.h"
#include "gfs_idx.h"
#include "gfs_grp.h"

#define REC_NAME_AV_CFG(mnt, p)      sprintf(p, "%s/av.cfg", mnt)
#define REC_NAME_AV_IDX(mnt, p)      sprintf(p, "%s/av.idx", mnt)
#define REC_NAME_AV_DATA(mnt, n, p)  sprintf(p, "%s/av%06d.dat", mnt, n)

#define REC_RWSET(fpic, frw)         ( (((fpic)&0x1) << 1) | ((frw)&0x1))
#define REC_RWGET(rwtype, fpic, frw) do{fpic = (((rwtype)>>1) & 0x1); frw = ((rwtype) & 0x1);}while(0) 

typedef struct rec_writer_s 
{
    int rwtype;
    grp_disk_t *gd;
    int fd;
    int ch, fmt, bkt_no, seg_cnt;
    gfs_vseg_t vseg;
    int new_seg;
    unsigned int flush_time;
    unsigned int idx_len, av_len;
    unsigned char *idx_data, *av_data;
}rec_writer_t;

int rec_writer_open(gfs_rec_rw_t *rw, rec_writer_t **writer);
int rec_writer_close(rec_writer_t *writer);
int rec_writer_write(rec_writer_t *writer,  gfs_frm_t *frm);

typedef struct rec_reader_s 
{
    int rwtype;
    grp_disk_t *gd;
    int fd;
    unsigned int off;
    unsigned int end;
    int idx_cnt;
    gfs_idx_t idxs[GFS_FILE_IDX_NUM];
}rec_reader_t;

int rec_reader_open(gfs_rec_rw_t *rw, rec_reader_t **reader, char *mnt);
int rec_reader_close(rec_reader_t *reader);
int rec_reader_read(rec_reader_t *reader, char *buf, int *size);
int rec_reader_seek_time(rec_reader_t *reader, int time);
int rec_reader_seek_pos(rec_reader_t *reader, int pos);

typedef struct pic_writer_s 
{
    int rwtype;
    grp_disk_t *gd;
    int fd;
    int bkt_no, seg_cnt;
    gfs_pseg_t pseg;
}pic_writer_t;

int pic_writer_open(gfs_rec_rw_t *rw, pic_writer_t **writer);
int pic_writer_close(pic_writer_t *writer);
int pic_writer_write(pic_writer_t *writer,  gfs_frm_t *frm);

typedef struct pic_reader_s 
{
    int rwtype;
    grp_disk_t *gd;
    int fd;
    unsigned int off;
    unsigned int end;
}pic_reader_t;

int pic_reader_open(gfs_rec_rw_t *rw, pic_reader_t **reader, char *mnt);
int pic_reader_close(pic_reader_t *reader);
int pic_reader_read(pic_reader_t *reader, char *buf, int *size);


#endif //~__gfs_rec_h__
