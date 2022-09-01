#include "gfs.h"
#include "gfs_disk.h"
#include "gfs_rec.h"
#include "printd.h"

int gfs_init(gfs_init_parm_t *parm)
{
    if(parm && parm->log)
      gfs_setlog(LOG_LEVEL_ERR, parm->log);
    disk_init();
    grp_init();
    return 0;
}

int gfs_uninit(void)
{
    grp_uninit();
    disk_uninit();
    return 0;
}

int gfs_disk_scan(gfs_disk_q_t *q)
{
    return disk_scan(q);
}
int gfs_disk_format(gsf_disk_f_t *f)
{
    return disk_format(f);
}

int gfs_grp_disk_add(gfs_disk_t *disk)
{
    int ret = 0;
    grp_disk_t *gd = NULL;
    
    ret = grp_get_disk(disk->grp_id, disk->disk_id, &gd);
    if(!gd)
    {
        return grp_disk_open(disk);
    }
    ret = GFS_ER_ADISK;
    return ret;
}
int gfs_grp_disk_del(int grp_id, int disk_id)
{
    int ret = 0;
    grp_disk_t *gd = NULL;
    ret = grp_get_disk(grp_id, disk_id, &gd);
    if(gd)
    {
        return grp_disk_close(gd);
    }
    
    return ret;
}

int gfs_grp_disk_each(int grp_id, gfs_gdisk_q_t *q)
{
    return grp_disk_each(grp_id, q);
}


int gsf_grp_idx_load(int grp_id)
{
    return grp_idx_load(grp_id);
}


int gfs_rec_query(gfs_rec_q_t *q)
{
    return grp_useg_query(q);
}

int gfs_rec_open(void **hdl, gfs_rec_rw_t *rw)
{
    int fpic, frw;
    fpic = rw->fpic;
    frw  = rw->frw;
    
    if(!fpic)
    {
        return (frw)?rec_writer_open(rw, (rec_writer_t **)hdl):
                        rec_reader_open(rw, (rec_reader_t **)hdl, NULL);
    }
    else
    {
        return (frw)?pic_writer_open(rw, (pic_writer_t **)hdl):
                        pic_reader_open(rw, (pic_reader_t **)hdl, NULL);
    }
    return -1;
}


int gfs_rec_open_r(void **hdl, gfs_rec_rw_t *rw, char *mnt)
{
    int fpic;
    fpic = rw->fpic;
    rw->frw = 0;    //force only read;
    if(!fpic)
    {
        return rec_reader_open(rw, (rec_reader_t **)hdl, mnt);
    }
    else
    {
        return pic_reader_open(rw, (pic_reader_t **)hdl, mnt);
    }
    return -1;
}


int gfs_rec_write(void *hdl, gfs_frm_t *frm)
{
    if(!hdl)
        return GFS_ER_PARM;
        
    int rwtype = *((int*)(hdl));
    int fpic, frw;
    REC_RWGET(rwtype, fpic, frw);
    frw = frw;
    if(!fpic)
    {
        return rec_writer_write((rec_writer_t *)hdl, frm);
    }
    else
    {
        return pic_writer_write((pic_writer_t *)hdl, frm);
    }
    return -1;
}
int gfs_rec_read(void *hdl, char *buf, int *size)
{
    if(!hdl)
        return GFS_ER_PARM;
        
    int rwtype = *((int*)(hdl));
    int fpic, frw;
    REC_RWGET(rwtype, fpic, frw);
    frw = frw;
    if(!fpic)
    {
        return rec_reader_read((rec_reader_t *)hdl, buf, size);
    }
    else
    {
        return pic_reader_read((pic_reader_t *)hdl, buf, size);
    }
    return -1;
}

int gfs_rec_seek_time(void *hdl, int time)
{
    if(!hdl)
        return GFS_ER_PARM;
        
    int rwtype = *((int*)(hdl));
    int fpic, frw;
    REC_RWGET(rwtype, fpic, frw);
    
    if(fpic == 0 && frw == 0)
    {
        return rec_reader_seek_time((rec_reader_t *)hdl, time);
    }
    return -1;
}
int gfs_rec_seek_pos(void *hdl, int pos)
{
    if(!hdl)
        return GFS_ER_PARM;
        
    int rwtype = *((int*)(hdl));
    int fpic, frw;
    REC_RWGET(rwtype, fpic, frw);
    
    if(fpic == 0 && frw == 0)
    {
        return rec_reader_seek_pos((rec_reader_t *)hdl, pos);
    }
    return -1;
}

int gfs_rec_close(void *hdl)
{
    if(!hdl)
        return GFS_ER_PARM;
        
    int rwtype = *((int*)(hdl));
    int fpic, frw;
    REC_RWGET(rwtype, fpic, frw);
    
    if(!fpic)
    {
        return (frw)?rec_writer_close((rec_writer_t *)hdl):
                        rec_reader_close((rec_reader_t *)hdl);
    }
    else
    {
        return (frw)?pic_writer_close((pic_writer_t *)hdl):
                        pic_reader_close((pic_reader_t *)hdl);
    }
    return -1;
}