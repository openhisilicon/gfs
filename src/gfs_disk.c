/* gfs ´ÅÅÌ²Ù×÷ */
#define _LARGEFILE64_SOURCE
#include <sys/mount.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/fs.h>

#include "list.h"

#include "gfs_disk.h"
#include "gfs_rec.h"
#include "scandisk.h"
#include "printd.h"

#define TRUE 1
#define FALSE 0

typedef struct disk_s
{
    struct list_head list;
    gfs_disk_t disk;
}disk_t;

typedef struct disk_mng_s 
{
    int max_disk_id;
    struct list_head disks;
}disk_mng_t;


static disk_mng_t mng;

int disk_init(void)
{
    mng.max_disk_id = 0;
    INIT_LIST_HEAD(&mng.disks);
    
    if(access(DISK_SCAN_MNT_DIR, 0) < 0)
        mkdir(DISK_SCAN_MNT_DIR, 0666);
        
    if(access(DISK_FORMAT_MNT_DIR, 0) < 0)
        mkdir(DISK_FORMAT_MNT_DIR, 0666);
    
    return 0;
}

int disk_uninit(void)
{
    disk_t *n, *nn;

    list_for_each_entry_safe(n, nn, &mng.disks, list)
    {
        list_del(&n->list);
        free(n);
    }
    return 0;
}


static int
valid_offset (int fd, long long offset)
{
  char ch;

  if (lseek64 (fd, offset, SEEK_SET) < 0)
    return FALSE;
  if (read (fd, &ch, 1) < 1)
    return FALSE;
  return TRUE;
}

static unsigned long long
count_blocks (char *filename)
{
  long long high, low;
  int fd;

  if ((fd = open (filename, O_RDONLY)) < 0)
    {
      perror (filename);
      exit (1);
    }

  /* first try SEEK_END, which should work on most devices nowadays */
  if ((low = lseek64(fd, 0, SEEK_END)) <= 0) {
      low = 0;
      for (high = 1; valid_offset (fd, high); high *= 2)
	  low = high;
      while (low < high - 1) {
	  const long long mid = (low + high) / 2;
	  if (valid_offset (fd, mid))
	      low = mid;
	  else
	      high = mid;
      }
      ++low;
  }

  close (fd);
  //printf("disk size %lld\n", low);
  return low / BLOCK_SIZE;
}

int scan_disk_cb(void *uargs, scandisk_info_t *info)
{
    printf("scan [dev:%s, bus:%d, hostno:%d, ma:%d, mi:%d, part:%d, target:%s]\n"
        , info->dev_name
        , info->bus_class
        , info->hostno
        , info->ma
        , info->mi
        , info->part_num
        , info->ptarget
        );
    
    disk_t *n;

    list_for_each_entry(n, &mng.disks, list)
    {
        if(strcmp(n->disk.dev_name, info->dev_name) == 0)
        {
            return 0;
        }
    }
    
    
    n = (disk_t *)calloc(1, sizeof(disk_t));
    n->disk.grp_id  = GFS_INVALID_VAL;
    n->disk.disk_id = GFS_INVALID_VAL;

    strncpy(n->disk.dev_name, info->dev_name, sizeof(n->disk.dev_name)-1);
    
    n->disk.size = count_blocks(info->dev_name)/(1024*1024/BLOCK_SIZE);
    
    umount2(DISK_SCAN_MNT_DIR, MNT_FORCE);
    if(mount(info->dev_name, DISK_SCAN_MNT_DIR, DISK_FS_TYPE, MS_NOATIME, NULL) == 0)
    {
        char filename[256];
        REC_NAME_AV_CFG(DISK_SCAN_MNT_DIR, filename);
        
        int fd;
        gfs_cfg_t cfg;
        memset(&cfg, 0, sizeof(gfs_cfg_t));
        
        if(access(filename, 0) == 0
            && (fd = open(filename, O_RDWR | O_CREAT, 0766)))
        {
            read(fd, &cfg, sizeof(gfs_cfg_t));
            
            n->disk.grp_id  = cfg.grp_id;
            n->disk.disk_id = cfg.disk_id;
            close(fd);
        }
        
        umount2(DISK_SCAN_MNT_DIR, MNT_FORCE);
    }
    
    if(n->disk.disk_id > mng.max_disk_id)
        mng.max_disk_id = n->disk.disk_id;
 
    list_add(&n->list, &mng.disks);

    return 0;
}


int disk_scan(gfs_disk_q_t *q)
{
    scandisk_op_t op;
    op.uargs = &mng;
    op.cb = scan_disk_cb;
    scan_disk(&op);
 
    disk_t *n;

    list_for_each_entry(n, &mng.disks, list)
    {
        q->cb(q, &n->disk);
    }
    
    return 0;
}

#if (defined __i386__ || defined __x86_64__)
    //#define _XOPEN_SOURCE 600
    //#include <fcntl.h>
    //#warning __X86_ARCH__
#elif (defined GSF_CPU_3536d)
    //#warning __ARM_ARCH__
    #include <sys/syscall.h>
    #ifdef SYS_fallocate
    int posix_fallocate(int fd, int offset, int len)
    {
        return syscall(SYS_fallocate, fd, 0, offset, 0, len, 0);
    }
    #endif
#endif


int disk_format(gsf_disk_f_t *f)
{
    gfs_disk_t *disk = f->disk;
    char filename[GFS_FILE_NAME_SIZE] = {0};
    
    disk->disk_id = (disk->disk_id != GFS_INVALID_VAL)?disk->disk_id:++mng.max_disk_id;
    disk->grp_id = (disk->grp_id != GFS_INVALID_VAL)?disk->grp_id:0;
    
    f->cb(f, 1);
    
    if(strlen(disk->dev_name))
    {
        // mkfs;
        char mkfs[512];
        sprintf(mkfs,
            "mke2fs "
            "%s "
            "-F "
            "-b "
            "4096 "
            "-O "
            "sparse_super,uninit_bg,flex_bg,extent,^has_journal,^resize_inode "
            "-E "
            "lazy_itable_init=1 "
            "-G "
            "4096 "
            "-L "
            "record "
            "-t "
            "ext4 ",
            disk->dev_name);
        printf("mkfs:[%s]\n", mkfs);
        system(mkfs);
        
        // mount;
        sprintf(disk->mnt_dir, "%s", DISK_FORMAT_MNT_DIR);
        umount2(disk->mnt_dir, MNT_FORCE);
        if(mount(disk->dev_name, disk->mnt_dir, DISK_FS_TYPE, MS_NOATIME, NULL) < 0)
        {
            return GFS_ER_MOUNT;
        }
        f->cb(f, 10);
    }
    
    if(!strlen(disk->mnt_dir))
    {
        return GFS_ER_PARM;
    }
    
    { // cfg;
        REC_NAME_AV_CFG(disk->mnt_dir, filename);
        int fd = open(filename, O_RDWR | O_CREAT, 0766);
        gfs_cfg_t cfg;
        cfg.disk_id = disk->disk_id;
        cfg.grp_id = disk->grp_id;
        write(fd, &cfg, sizeof(gfs_cfg_t));
        close(fd);
        f->cb(f, 11);
    }
    
    { // idx;
        REC_NAME_AV_IDX(disk->mnt_dir, filename);
        int fd = open(filename, O_RDWR | O_CREAT, 0766);
        int i = 0;
        char *hdr_buf[GFS_IDX_HDR_SIZE] = {0};
        gfs_hdr_t *hdr = (gfs_hdr_t*)hdr_buf;
        hdr->used_cnt = 0;
        hdr->bkt_total= disk->size/(GFS_FILE_SIZE/(1024*1024)) * 0.95;
        hdr->bkt_size = GFS_FILE_SIZE;
        hdr->bkt_next = 0;
        
        printf("\n------------\n");
        printf("disk_id: %d\n", disk->disk_id);
        printf("used_cnt: %d\n", hdr->used_cnt);
        printf("bkt_total: %d\n", hdr->bkt_total);
        printf("bkt_size: %d\n", hdr->bkt_size);
        printf("bkt_next: %d\n", hdr->bkt_next);
        printf("------------\n");
        
        for(i = 0; i < GFS_CH_MAX_NUM; i++)
        {
            hdr->bkt_curr[i] = GFS_INVALID_VAL;
        }
        write(fd, hdr_buf, GFS_IDX_HDR_SIZE);
        for(i = 0; i < hdr->bkt_total; i++)
        {
            char bkt_buf[GFS_IDX_BKT_SIZE] = {0};
            write(fd, bkt_buf, GFS_IDX_BKT_SIZE);
        }
        close(fd);
        f->cb(f, 12);
        
        // dat;
        for(i = 0; i < hdr->bkt_total; i++)
        {
            f->cb(f, 12 + i*((100-12)/(hdr->bkt_total*1.0)));
            REC_NAME_AV_DATA(disk->mnt_dir, i, filename);
            int fd = open(filename, O_RDWR | O_CREAT, 0766);
            posix_fallocate(fd, 0, GFS_FILE_SIZE);
            close(fd);
        }
    }
    
    sync();
    
    if(strlen(disk->dev_name))
    {
        umount2(disk->mnt_dir, MNT_FORCE);
    }
    
    f->cb(f, 100);
    return 0;
}

