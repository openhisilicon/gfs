#ifndef __gfs_grp_h__
#define __gfs_grp_h__

/* gfs ≈Ã◊Èπ‹¿Ì 
 * qq: 25088970 maohw
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#include "list.h"
#include "btree.h"

#include "gfs.h"

enum {
    GRP_STAT_NONE = 0,
    GRP_STAT_INIT = 1,
    GRP_STAT_LOAD = 2,
};


struct grp_disk_s;

typedef struct grp_s {
    struct list_head disks;
    pthread_mutex_t lock;
    int stat;
    struct grp_disk_s *curr;
    struct btree_head64 bt_av;
    struct btree_head64 bt_pic;
}grp_t;


typedef struct grp_disk_s {
    struct list_head list;
    int fd;
    grp_t *grp;
    gfs_disk_t disk;
    struct list_head uvsegs;
    struct list_head upsegs;
    gfs_hdr_t *av_hdr;
}grp_disk_t;

typedef struct grp_uvseg_s {
    struct list_head list;
    gfs_uvseg_t uvseg;
}grp_uvseg_t;

typedef struct grp_upseg_s {
    struct list_head list;
    gfs_upseg_t upseg;
}grp_upseg_t;


int grp_init(void);
int grp_uninit(void);

int grp_disk_open(gfs_disk_t *disk);
int grp_disk_close(grp_disk_t *gd);
int grp_idx_load(int grp_id);

int grp_get_curr_disk(int grp_id, int ch, grp_disk_t **gd, int *bkt_no);
int grp_get_disk(int grp_id, int disk_id, grp_disk_t **gd);
int grp_rel_disk(grp_disk_t *gd);
int grp_disk_each(int grp_id, gfs_gdisk_q_t *q);

int grp_useg_query(gfs_rec_q_t *q);
int grp_useg_sync(grp_disk_t *gd, int bkt_no, int fpic, void *seg);

#endif //~__gfs_grp_h__