#if 1

/* gfs 测试 
 * qq: 25088970 maohw
 */
 
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#include "gfs.h"

static int __load = 0;

void usage()
{
    printf("scan   [ find dev and format and load ]\n"
           "format [ format dir ]                  \n"
           "load   [ load dir ]                    \n"
           "grp    [ each grp disk]                \n"
           "wrec   [ write rec ]                   \n"
           "wpic   [ write pic ]                   \n"
           "qrec   [ query rec ]                   \n"
           "qpic   [ query pic ]                   \n"
           "rrec   [ read rec, need qrec ]         \n"
           "q      [ quit ]                        \n"
           );
}

int format_cb(struct gsf_disk_f_s *owner, int percent)
{
    printf("format disk_id:%d, percent:%d%%\n", owner->disk->disk_id, percent);
    return 0;
}


int scan_cb(struct gfs_disk_q_s *owner, gfs_disk_t *disk)
{
    printf("disk [grp:%d, id:%d, size:%dM, dev:%s, mnt:%s]\n"
        , disk->grp_id
        , disk->disk_id
        , disk->size
        , disk->dev_name
        , disk->mnt_dir
        );

    {
      printf("format dev:%s choose Y/n !\n", disk->dev_name);
      char c = (char)getchar(); 
      if(c != 'Y')
        return 0;
      printf(">>>>>>>>>> format dev:%s !\n", disk->dev_name);
    }
    
    #if 1
    if(disk->grp_id == -1)
    {
        gsf_disk_f_t f;
        f.disk = disk;
        f.uargs = NULL;
        f.cb = format_cb;
        gfs_disk_format(&f);
    }
    
    {
        __load = 1;
        char mnt_dir[256];
        char cmd[256];
        sprintf(mnt_dir, "/mnt/rec%08d", disk->disk_id);
        sprintf(cmd, "mkdir -p %s", mnt_dir);
        system(cmd);
        strcpy(disk->mnt_dir, mnt_dir);
        gfs_grp_disk_add(disk);
    }
    #endif
    return 0;
}

int scan()
{
    gfs_disk_q_t q;
    q.cb = scan_cb;
    gfs_disk_scan(&q);
    
    gsf_grp_idx_load(0);
    
    return 0;
}


char *localtime_str(int t, char s[32])
{
    struct timeval tv;
    struct tm tm;
    tv.tv_sec=t;
    tv.tv_usec=0;
    localtime_r(&tv.tv_sec,&tm);
    
    sprintf(s, "%04d-%02d-%02d %02d:%02d:%02d"
            , tm.tm_year+1900
            , tm.tm_mon+1 
            , tm.tm_mday
            , tm.tm_hour
            , tm.tm_min
            , tm.tm_sec);
    return s;
}

static gfs_uvseg_t last_uvseg;
static gfs_upseg_t last_upseg;
int qcb(struct gfs_rec_q_s *owner, void *useg)
{
    int *cnt = (int*)(owner->uargs);
    
    if(!owner->fpic)
    {
        gfs_uvseg_t *uvseg = (gfs_uvseg_t*)(useg);
        
        last_uvseg = *uvseg;
        #ifdef DEBUG
        char btime[32];
        char etime[32];
        printf("disk_id:%d, bkt_id:%d, ch:%d, btime:%s, etime:%s\n"
            , uvseg->disk_id
            , uvseg->bkt_id
            , uvseg->vseg.ch
            , localtime_str(uvseg->vseg.btime, btime)
            , localtime_str(uvseg->vseg.etime, etime));
        #endif
    }
    else
    {
        gfs_upseg_t *upseg = (gfs_upseg_t*)(useg);
        
        last_upseg = *upseg;
        #ifdef DEBUG
        char btime[32];
        printf("disk_id:%d, bkt_id:%d, ch:%d, btime:%s, ms5:%d\n"
            , upseg->disk_id
            , upseg->bkt_id
            , upseg->pseg.ch
            , localtime_str(upseg->pseg.btime, btime)
            , upseg->pseg.ms5);
        #endif
    }
    
    if((*cnt)++ >= 10)
    {
        //return -1;
    }
    return 0;
}


int query(int fpic)
{
    char btime[32];
    char etime[32];
    
    gfs_rec_q_t q;
    q.fpic = fpic;
    q.grp_id = 0;
    if(q.fpic)
    {
        q.btime = time(NULL) - 10;
        q.etime = q.btime + 2;
    }
    else
    {
        q.btime = time(NULL) - 10;
        q.etime = q.btime + 10;
    }
    
    q.btime = 0;
    q.etime = time(NULL);
    
    int cnt = 0;
    q.uargs = &cnt;
    q.cb = qcb;
    
    gfs_rec_query(&q);
    printf("[%s -> %s], fpic:%d, cnt:%d\n"
        , localtime_str(q.btime, btime)
        , localtime_str(q.etime, etime)
        , fpic
        , cnt);
    return 0;
}


#define TEST_DIR "/mnt/hgfs/works/gfs/rec%02d"

int format()
{
    static int format = 0;
    
    if(format)
        return 0;
    format = 1;
    
    int i = 0;
    gfs_disk_t disk;
    char mnt_dir[256];
    char cmd[256];
    for(i = 0; i < 4; i++)
    {
        sprintf(mnt_dir, TEST_DIR, i+1);
        sprintf(cmd, "mkdir -p %s", mnt_dir);
        system(cmd);
        
        disk.grp_id = GFS_INVALID_VAL;
        disk.disk_id = GFS_INVALID_VAL;
        disk.size = 1024;
        disk.dev_name[0] = '\0';
        strcpy(disk.mnt_dir, mnt_dir);

        gsf_disk_f_t f;
        f.disk = &disk;
        f.uargs = NULL;
        f.cb = format_cb;
        gfs_disk_format(&f);
    }
    return 0;
}

int load()
{
    int i = 0;
    gfs_disk_t disk;
    
    if(__load)
        return 0;
    __load = 1;
    
    for(i = 0; i < 4; i++)
    {
        disk.grp_id = 0;
        disk.disk_id = i+1;
        disk.size = 1024;
        disk.dev_name[0] = '\0';
        sprintf(disk.mnt_dir, TEST_DIR, i+1);
        
        gfs_grp_disk_add(&disk);
    }
    gsf_grp_idx_load(0);
    
    return 0;
}


int grp_cb(struct gfs_gdisk_q_s *owner, gfs_hdr_t *hdr)
{
    printf("grp used_cnt:%d, bkt_total:%d, bkt_size:%dM, bkt_next:%d\n"
        , hdr->used_cnt
        , hdr->bkt_total
        , hdr->bkt_size/(1024*1024)
        , hdr->bkt_next
        );
    return 0;
}

int grp()
{
    gfs_gdisk_q_t q;
    q.uargs = NULL;
    q.cb = grp_cb;
    return gfs_grp_disk_each(0, &q);
}

#define TEST_OUT "/mnt/hgfs/works/gfs/of.data"

int reader()
{
    void *r;
    int ret = 0;
    gfs_rec_rw_t rw;
    rw.fpic = 0;
    rw.frw = 0;
    rw.grp_id = 0;
    rw.parm.rv = last_uvseg;

    ret = gfs_rec_open(&r, &rw);
    if(ret < 0)
    {
        printf("open ret:%d\n", ret);
        return -1;
    }
    int osize = 0;
    int of = open(TEST_OUT, O_RDWR|O_CREAT, 0777);
    if(of <= 0)
      printf("open of err.\n");
      
    char buf[64*1024];
    int len = sizeof(buf);
    int cnt = 0;
    while((ret = gfs_rec_read(r, buf, &len)) > 0)
    {
      #if 0
        if(cnt == 30)
            gfs_rec_seek_time(r, rw.parm.rv.vseg.btime + 3); // 指定时间戳
        else if(cnt == 60)
            gfs_rec_seek_pos(r, 5);     // 向前 5 个关键帧
        else if(cnt == 90)
            gfs_rec_seek_pos(r, -3);    // 向后 3 个关键帧
          
        //printf("read ret:%d, len:%d\n", ret, len);
      #else
        if(of > 0)
        {
          write(of, buf, len);
          osize+=len;
        }
      #endif
        cnt++;
    }
    
    if(of > 0)
    {
      printf("osize:%d\n", osize);
      close(of);
    } 
    printf("close ret:%d\n", ret);
    gfs_rec_close(r);
    return 0;
}

static pthread_t write_tid = 0;
static int write_runing = 1;
void* write_task(void *parm)
{
    int i = 0, ret = 0;
    int fpic = (int)(parm);
    int n = (!fpic)?2:1;
    
    //n = 1;
    srand(time(NULL));
    
    do{
        int osize[2] = {0};
        void *w[2] = {NULL,NULL};
        
        for(i = 0; i < n; i++)
        {
            gfs_rec_rw_t rw;
            rw.fpic = fpic;
            rw.frw = 1;
            rw.grp_id = 0;
            rw.parm.w.ch = i;
            rw.parm.w.fmt = 0;
            ret = gfs_rec_open(&w[i], &rw);
            assert(ret == 0);
        }
        
        #define BUF_MAX 256  //k
        char frm_buf[BUF_MAX*8192];
        
        for(i = 0; i < 200; i++)
        {
            int ch = i%n;
            
            if(!write_runing)
                break;
            
            gfs_frm_t frm;
            
            if(!fpic)
                frm.widx = (i%15==0)?1:0;
            else
                frm.widx = ch;

            unsigned char c = rand()%BUF_MAX;
            
            struct timeval tv;
            gettimeofday(&tv, NULL);
            frm.time = tv.tv_sec;
            frm.time_ms = tv.tv_usec/1000;
            frm.tags = 0;
            frm.size = c*8192;
            frm.data = frm_buf;
            memset(frm.data, c, frm.size);
            osize[ch] += frm.size;
            ret = gfs_rec_write((!fpic)?w[ch]:w[0], &frm);
            if(ret < 0)
            {
                printf("gfs_rec_write ch:%d, ret:%d\n", ch, ret);
                break;
            }
            //printf("gfs_rec_write ch:%d, frm.size:%d, frm.data[%02X]\n", ch, frm.size, c);

            usleep(20*1000);
        }
        
        for(i = 0; i < n; i++)
        {
            printf("osize[%d]:%d\n", i, osize[i]);
            ret = gfs_rec_close(w[i]);
        }
        

    }while(write_runing);
    
    return NULL;
}


int main(int argc, char *argv[])
{
    char cmd[256];
    char tmp[256];
    usage();
    
    gfs_init(NULL);
    
    while(1)
    {
        fgets(tmp, sizeof(tmp), stdin);
        tmp[strlen(tmp)-1] = '\0';
        if(strlen(tmp))
            strcpy(cmd, tmp);
        else
            usage();
            
        printf("cmd:[%s]\n", cmd);
        
        if(strstr(cmd, "scan"))
        {
            scan();
        }
        else if(strstr(cmd, "format"))
        {
            format();
        }
        else if(strstr(cmd, "load"))
        {
            load();
        }
        else if(strstr(cmd, "grp"))
        {
            grp();
        }
        else if(strstr(cmd, "qpic"))
        {
            query(1);
        }
        else if(strstr(cmd, "qrec"))
        {
            query(0);
        }
        else if(strstr(cmd, "wpic"))
        {
            if(!write_tid)
            {
                int fpic = 1;
                pthread_create(&write_tid, NULL, write_task, (void*)fpic);
            }
        }
        else if(strstr(cmd, "wrec"))
        {
            if(!write_tid)
            {
                int fpic = 0;
                pthread_create(&write_tid, NULL, write_task, (void*)fpic);
            }
        }
        else if(strstr(cmd, "rrec"))
        {
            reader();
        }
        
        else if(strstr(cmd, "q"))
        {
            if(write_tid)
            {
                write_runing = 0;
                pthread_join(write_tid, NULL);
            }
            
            break;
        }
    }
    
    gfs_uninit();
    
    return 0;
}


#endif
