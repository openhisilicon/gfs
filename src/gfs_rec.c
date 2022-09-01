/* gfs Â¼Ïñ²Ù×÷ */
#include "gfs_rec.h"
#include "printd.h"

#define REC_FILE_END() (w->vseg.dat_end + w->av_len + GFS_REC_MAX_SIZE > GFS_FILE_SIZE \
                        || w->vseg.idx_end >= GFS_FILE_IDX_SIZE \
                        || w->seg_cnt >= GFS_VSEG_MAX_NUM)

#define PIC_FILE_END() (w->pseg.dat_end + GFS_PIC_MAX_SIZE > GFS_FILE_SIZE \
                        || w->seg_cnt >= GFS_PSEG_MAX_NUM)

// add parm fsize, return flush size;
static int writer_flush(rec_writer_t *w, unsigned int fsize);
// add parm off, return append size;
static int writer_append(rec_writer_t *w, gfs_frm_t *frm, unsigned int off);

int rec_writer_open(gfs_rec_rw_t *rw, rec_writer_t **writer)
{
    int ret = 0, bkt_no = 0;
    char avfile[GFS_FILE_NAME_SIZE] = {0};
    
    grp_disk_t *gd = NULL;
    ret = grp_get_curr_disk(rw->grp_id, rw->parm.w.ch, &gd, &bkt_no);
    if(ret < 0)
    {
        return GFS_ER_NDISK;
    }

    rec_writer_t *w = (rec_writer_t*)calloc(1, sizeof(rec_writer_t));
    w->rwtype = REC_RWSET(rw->fpic, rw->frw);
    w->ch  = rw->parm.w.ch;
    w->fmt = rw->parm.w.fmt;
    
    REC_NAME_AV_DATA(gd->disk.mnt_dir, bkt_no, avfile);
    w->gd = gd;
    w->bkt_no = bkt_no;
    w->fd = open(avfile, O_RDWR|O_CREAT, 0777);
    
    w->idx_len = 0;
    w->idx_data = (unsigned char*)malloc(GFS_FILE_IDX_SIZE);
    ret = pread(w->fd, w->idx_data, GFS_FILE_IDX_SIZE, 0);
    
    w->av_len  = 0;
    w->av_data = (unsigned char*)malloc(GFS_REC_BUF_SIZE);
    
    gfs_bkt_t *bkt = (gfs_bkt_t *)malloc(GFS_IDX_BKT_SIZE);
    
    ret = idx_bkt_seg_read(gd->fd, bkt_no, bkt);
    w->seg_cnt = bkt->seg_cnt;
    if(w->seg_cnt > 0)
    {
        gfs_vseg_t *vseg = (gfs_vseg_t*)(bkt+1);
        w->vseg.idx_end = w->vseg.idx_beg = vseg[w->seg_cnt-1].idx_end;
        w->vseg.dat_end = w->vseg.dat_beg = GFS_ALIGN_4K(vseg[w->seg_cnt-1].dat_end);
    }
    else
    {
        w->vseg.idx_end = w->vseg.idx_beg = 0;
        w->vseg.dat_end = w->vseg.dat_beg = GFS_FILE_IDX_SIZE;
    }
#ifdef DEBUG
    printf("open ch:%d, bkt_no:%d, idx_beg:%X, dat_beg:%X [seg_cnt:%d]\n"
        , w->ch
        , w->bkt_no
        , w->vseg.idx_beg
        , w->vseg.dat_beg
        , w->seg_cnt);
#endif
    free(bkt);
    
    *writer = w;
    return 0;
}

int rec_writer_close(rec_writer_t *writer)
{
    int whdr = 0;
    rec_writer_t *w = writer;
    
    whdr = REC_FILE_END()?1:0;
    
    writer_flush(w, w->av_len);
#ifdef DEBUG
    printf("close ch:%d, bkt_no:%d, idx_end:%X, dat_end:%X <= file_size:%X [seg_cnt:%d]\n"
        , w->ch
        , w->bkt_no
        , w->vseg.idx_end
        , w->vseg.dat_end
        , GFS_FILE_SIZE
        , w->seg_cnt);
    assert(w->vseg.dat_end <= GFS_FILE_SIZE);
#endif
    if(whdr)
    {
        w->gd->av_hdr->bkt_curr[w->ch] = GFS_INVALID_VAL;
        idx_hdr_write(w->gd->fd, w->gd->av_hdr);
    }
    
    grp_rel_disk(w->gd);
    close(w->fd);
    free(w->idx_data);
    free(w->av_data);
    free(w);
    
    return 0;
}




static int writer_flush(rec_writer_t *w, unsigned int fsize)
{
    if(w->av_len == 0 && w->idx_len == 0)
    {
        return 0;
    }
    
    // left data;
    int lsize = w->av_len - fsize;
    w->av_len = (lsize > 0)?lsize:0;

    if(fsize)
    {
        // write data;
        pwrite(w->fd, w->av_data, fsize, w->vseg.dat_end);
        w->vseg.dat_end += fsize;
        #if 0//def DEBUG
        printf("flush ch:%d, dat_beg:%X, dat_end:%X, size:%X, GFS_FILE_SIZE:%X\n"
            , w->ch
            , w->vseg.dat_beg
            , w->vseg.dat_end
            , w->vseg.dat_end - w->vseg.dat_beg
            , GFS_FILE_SIZE);
        #endif
        // left data;
        if(w->av_len > 0)
        {
          memmove(w->av_data, &w->av_data[fsize], w->av_len);
        }
    }
    if(w->idx_len)
    {
        // write frm idx;
        pwrite(w->fd, w->idx_data, w->idx_len, w->vseg.idx_end);
        w->vseg.idx_end += w->idx_len;
        #if 0//def DEBUG
        printf("flush ch:%d, idx_beg:%X, idx_end:%X, idx_num:%d\n"
            , w->ch
            , w->vseg.idx_beg
            , w->vseg.idx_end
            , (w->vseg.idx_end-w->vseg.idx_beg)/sizeof(gfs_idx_t));
        #endif
        w->idx_len = 0;
    }
    
    //write bkt;
    if(w->new_seg == 0)
    {
        w->new_seg = 1;
        w->seg_cnt++;
        
        gfs_bkt_t bkt;
        bkt.seg_cnt = w->seg_cnt;
        bkt.ferr = 0;
        bkt.flock = 0;
        bkt.fpic = 0;
        idx_bkt_write(w->gd->fd, w->bkt_no, &bkt);
    }
    
    //write idx;
    w->vseg.ch = w->ch;
    idx_seg_write(w->gd->fd, w->bkt_no, w->seg_cnt-1, 0, &w->vseg);
    
    //sync bt;
    grp_useg_sync(w->gd, w->bkt_no, 0, &w->vseg);
        
    return fsize;
}

static int writer_append(rec_writer_t *w, gfs_frm_t *frm, unsigned int off)
{
    int ret = 0;
    int lsize = GFS_REC_BUF_SIZE - w->av_len;
    int asize = ((frm->size - off) > lsize)?lsize:(frm->size - off);

    //append data;
    memcpy(&w->av_data[w->av_len], frm->data, asize);
    if(frm->widx && off == 0)
    {
        //append idx for first packet;
        gfs_idx_t *idx = (gfs_idx_t*)(&w->idx_data[w->idx_len]);
        idx->time = frm->time;
        idx->off  = w->vseg.dat_beg + w->av_len;
        w->idx_len += sizeof(gfs_idx_t);
    }
    w->vseg.btime = (w->vseg.btime==0)?frm->time:w->vseg.btime;
    w->vseg.etime = frm->time;
    w->av_len += asize;
    
    return asize;
}

int rec_writer_write(rec_writer_t *writer,  gfs_frm_t *frm)
{
    int ret = 0;
    rec_writer_t *w = writer;

    if(frm->size > GFS_REC_MAX_SIZE)
      return GFS_ER_BIGFRM;

    if(REC_FILE_END())
    {
        return ( w->vseg.idx_end >= GFS_FILE_IDX_SIZE)?GFS_ER_IEOF:GFS_ER_DEOF;
    }
    
    if(abs(time(NULL) - w->flush_time) >= 6
        || w->av_len + frm->size > GFS_REC_BUF_SIZE)
    {
        w->flush_time = time(NULL);

        unsigned int asize = 0;
        while((asize += writer_append(w, frm, asize)) < frm->size)
        {
          writer_flush(w, GFS_ROUND_4K(w->av_len));
        }
    }
    else
    {
        writer_append(w, frm, 0);
    }
    
    return ret;
}

int rec_reader_open(gfs_rec_rw_t *rw, rec_reader_t **reader, char *mnt)
{
    int ret = 0;
    char avfile[GFS_FILE_NAME_SIZE] = {0};
    
    grp_disk_t *gd = NULL;
    
    char *mnt_dir = NULL;
    
    if(mnt && strlen(mnt))
    {
        mnt_dir = mnt;
    }
    else 
    {
        ret = grp_get_disk(rw->grp_id, rw->parm.rv.disk_id, &gd);
        if(ret < 0)
        {
            return GFS_ER_NDISK;
        }
        mnt_dir = gd->disk.mnt_dir;
    }
    
    rec_reader_t *r = (rec_reader_t*)calloc(1, sizeof(rec_reader_t));
    
    r->rwtype = REC_RWSET(rw->fpic, rw->frw);
    
    REC_NAME_AV_DATA(mnt_dir,  rw->parm.rv.bkt_id, avfile);
    r->gd = gd;
    r->fd = open(avfile, O_RDONLY, 0777);
    r->off = rw->parm.rv.vseg.dat_beg;
    r->end = rw->parm.rv.vseg.dat_end;
    r->idx_cnt = (rw->parm.rv.vseg.idx_end-rw->parm.rv.vseg.idx_beg)/sizeof(gfs_idx_t);
    
    pread(r->fd, r->idxs, r->idx_cnt*sizeof(gfs_idx_t), rw->parm.rv.vseg.idx_beg);
#if 1
    int i = 0;
    for(i = 0; i < r->idx_cnt; i++)
    {
        printf("idxs[%d]: time:%d, off:0x%X\n", i, r->idxs[i].time, r->idxs[i].off);
    }
#endif
    *reader = r;
    return 0;
}

int rec_reader_close(rec_reader_t *reader)
{
    grp_rel_disk(reader->gd);
    close(reader->fd);
    free(reader);
    return 0;
}

int rec_reader_read(rec_reader_t *reader, char *buf, int *size)
{
    int ret = 0;
    
    *size = (reader->off + *size > reader->end)?(reader->end-reader->off):(*size);
    
    printf("read off:0x%X, size:%d, end:0x%X\n", reader->off, *size, reader->end);
    
    if(reader->off >= reader->end)
    {
        return GFS_ER_DEOF;
    }
    
    ret = pread(reader->fd, buf, *size, reader->off);
    if(ret >= 0)
    {
        reader->off += ret;
        *size = ret;
    }
    else
    {
        *size = 0;
    }
    return ret;
}


#define BINARY_SEARCH(s, e, n, key) \
({\
    int left = 0, right = n - 1, mid = 0;\
    mid = ( left + right ) / 2;\
    while( left < right && s[mid].e != key )\
    {\
        if( s[mid].e < key )\
            left = mid + 1;\
        else if( s[mid].e > key )\
            right = mid - 1;\
        mid = ( left + right ) / 2;\
    }\
    /*if( s[mid].e == key ) */\
        mid;\
})

int rec_reader_seek_time(rec_reader_t *reader, int time)
{
    int i = BINARY_SEARCH(reader->idxs, time, reader->idx_cnt, time);
    
    reader->off = reader->idxs[i].off;
    printf("seek_time: time:%d, idxs[%d][time:%d, off:0x%X]\n"
        , time, i, reader->idxs[i].time, reader->idxs[i].off);
    return 0;
}
int rec_reader_seek_pos(rec_reader_t *reader, int pos)
{
    int i = BINARY_SEARCH(reader->idxs, off, reader->idx_cnt, reader->off) + pos;
    
    if(i >= 0  && i < reader->idx_cnt)
    {
        reader->off = reader->idxs[i].off;
        printf("seek_pos: pos:%d, idxs[%d][time:%d, off:0x%X]\n"
            , pos, i, reader->idxs[i].time, reader->idxs[i].off);
    }
    else
    {
        printf("seek_pos: err pos:%d, i: %d, idx_cnt:%d\n", pos, i, reader->idx_cnt);
    }
    return 0;
}

int pic_writer_open(gfs_rec_rw_t *rw, pic_writer_t **writer)
{
    int ret = 0, bkt_no = 0;
    char avfile[GFS_FILE_NAME_SIZE] = {0};
    
    grp_disk_t *gd = NULL;
    ret = grp_get_curr_disk(rw->grp_id, GFS_CH_PIC_NO, &gd, &bkt_no);
    if(ret < 0)
    {
        return GFS_ER_NDISK;
    }

    pic_writer_t *w = (pic_writer_t*)calloc(1, sizeof(pic_writer_t));
    w->rwtype = REC_RWSET(rw->fpic, rw->frw);
    
    REC_NAME_AV_DATA(gd->disk.mnt_dir, bkt_no, avfile);
    w->gd = gd;
    w->bkt_no = bkt_no;
    w->fd = open(avfile, O_RDWR|O_CREAT, 0777);
    
    gfs_bkt_t *bkt = (gfs_bkt_t *)malloc(GFS_IDX_BKT_SIZE);
    
    ret = idx_bkt_seg_read(gd->fd, bkt_no, bkt);
    w->seg_cnt = bkt->seg_cnt;
    if(w->seg_cnt > 0)
    {
        gfs_pseg_t *pseg = (gfs_pseg_t*)(bkt+1);
        w->pseg.dat_end = w->pseg.dat_beg = GFS_ALIGN_4K(pseg[w->seg_cnt-1].dat_end);
    }
    else
    {
        w->pseg.dat_end = w->pseg.dat_beg = GFS_FILE_IDX_SIZE;
    }
#ifdef DEBUG
    printf("open bkt_no:%d, dat_beg:%X [seg_cnt:%d]\n"
        , w->bkt_no
        , w->pseg.dat_beg
        , w->seg_cnt);
#endif
    free(bkt);
    
    *writer = w;
    
    return 0;
}
int pic_writer_close(pic_writer_t *writer)
{
    pic_writer_t *w = writer;

#ifdef DEBUG
    printf("close bkt_no:%d, dat_end:%X [seg_cnt:%d]\n"
        , w->bkt_no
        , w->pseg.dat_end
        , w->seg_cnt);
#endif
    if(PIC_FILE_END())
    {
        w->gd->av_hdr->bkt_curr[GFS_CH_PIC_NO] = GFS_INVALID_VAL;
        idx_hdr_write(w->gd->fd, w->gd->av_hdr);
    }
    
    grp_rel_disk(w->gd);
    close(w->fd);
    free(w);
    
    return 0;
}


int pic_writer_write(pic_writer_t *writer,  gfs_frm_t *frm)
{
    int ret = 0;
    pic_writer_t *w = writer;
    
    if(frm->size > GFS_PIC_MAX_SIZE)
      return GFS_ER_BIGFRM;
    
    if(PIC_FILE_END())
    {
        return (w->seg_cnt >= GFS_PSEG_MAX_NUM)?GFS_ER_SEOF:GFS_ER_DEOF;
    }
    
    if(frm->size <= 0)
    {
        return GFS_ER_PARM;
    }

    // write data;
    pwrite(w->fd, frm->data, frm->size, w->pseg.dat_end);
    w->pseg.dat_end = w->pseg.dat_beg + frm->size;

    // write idx;
    w->seg_cnt++;
    w->pseg.ch = frm->widx;
    w->pseg.btime = frm->time;
    w->pseg.ms5 = frm->time_ms/5; // Í¼Æ¬Ê±¼ä´Á 5ms ¾«¶È´æ´¢;
    idx_seg_write(w->gd->fd, w->bkt_no, w->seg_cnt-1, 1, &w->pseg);

    gfs_bkt_t bkt;
    bkt.seg_cnt = w->seg_cnt;
    bkt.ferr  = 0;
    bkt.flock = 0;
    bkt.fpic  = 1;
    idx_bkt_write(w->gd->fd, w->bkt_no, &bkt);

    //sync bt;
    grp_useg_sync(w->gd, w->bkt_no, 1, &w->pseg);
    
    
    w->pseg.dat_beg = w->pseg.dat_end;
    
    return ret;
}

int pic_reader_open(gfs_rec_rw_t *rw, pic_reader_t **reader, char *mnt)
{
    int ret = 0;
    char avfile[GFS_FILE_NAME_SIZE] = {0};
    
    grp_disk_t *gd = NULL;
    
    char *mnt_dir = NULL;
    
    if(mnt && strlen(mnt))
    {
        mnt_dir = mnt;
    }
    else 
    {
        ret = grp_get_disk(rw->grp_id, rw->parm.rp.disk_id, &gd);
        if(ret < 0)
        {
            return GFS_ER_NDISK;
        }
        mnt_dir = gd->disk.mnt_dir;
    }
    
    pic_reader_t *r = (pic_reader_t*)calloc(1, sizeof(pic_reader_t));
    
    r->rwtype = REC_RWSET(rw->fpic, rw->frw);
    
    REC_NAME_AV_DATA(mnt_dir,  rw->parm.rp.bkt_id, avfile);
    r->gd = gd;
    r->fd = open(avfile, O_RDONLY, 0777);
    r->off = rw->parm.rp.pseg.dat_beg;
    r->end = rw->parm.rp.pseg.dat_end;

    *reader = r;
    return 0;
}
int pic_reader_close(pic_reader_t *reader)
{
    grp_rel_disk(reader->gd);
    close(reader->fd);
    free(reader);
    return 0;
}
int pic_reader_read(pic_reader_t *reader, char *buf, int *size)
{
    int ret = 0;
    
    *size = (reader->off + *size > reader->end)?(reader->end-reader->off):(*size);
    
    printf("read off:0x%X, size:%d, end:0x%X\n", reader->off, *size, reader->end);
    
    if(reader->off >= reader->end)
    {
        return GFS_ER_DEOF;
    }
    
    ret = pread(reader->fd, buf, *size, reader->off);
    if(ret >= 0)
    {
        reader->off += ret;
        *size = ret;
    }
    else
    {
        *size = 0;
    }
    return ret;
}