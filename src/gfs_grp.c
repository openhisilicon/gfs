#include <sys/mount.h>
#include <linux/fs.h>

#include "gfs_grp.h"
#include "gfs_rec.h"
#include "gfs_disk.h"
#include "printd.h"

static inline u64 left64(u64 a, int len)
{
    u32 *p = (u32*) &a;
    if (len <32)
    {
        *(p+1) <<= len;
        u32 tmp = (*p) >> (32-len);
        *(p+1) |= tmp;
        *p <<= len;
    }
    else
    {
        *(p+1) = *p;
        *p = 0x00000000;
        *(p+1) <<= (len-32);
    }
    return a;
}

static inline u64 right64(u64 a, int len)
{
    u32 *p = (u32*) &a;
    if (len<32)
    {
        *p >>= len;
        u32 tmp = *(p+1) << (32-len);
        *p |= tmp;
        *(p+1) >>= len;
    }
    else
    {
        *p = *(p+1);
        *(p+1) = 0x00000000;
        *p >>= (len-32);
    }
    return a;
}


static grp_t grps[GFS_GRP_MAX_NUM];

int grp_init(void)
{
    int i = 0;
    grp_t *grp;
    
    for(i = 0; i < GFS_GRP_MAX_NUM; i++)
    {
        grp = &grps[i];
        grp->stat = GRP_STAT_INIT;
        INIT_LIST_HEAD(&grp->disks);
        btree_init64(&grp->bt_av);
        btree_init64(&grp->bt_pic);
        //pthread_mutex_init(&grp->lock, NULL);
        grp->curr = NULL;
    }
    return 0;
}
int grp_uninit(void)
{
    int i = 0;
    grp_t *grp;
    
    for(i = 0; i < GFS_GRP_MAX_NUM; i++)
    {
        grp = &grps[i];

        grp->stat = GRP_STAT_NONE;

        grp_disk_t *n, *nn;
    
        list_for_each_entry_safe(n, nn, &grp->disks, list)
        {
            grp_disk_close(n);
        }

        u64 key;
        void *node;

    	btree_for_each_safe64(&grp->bt_av, key, node)
    	{
    		btree_remove64(&grp->bt_av, key);
    		free(node);
    	}
    	btree_destroy64(&grp->bt_av);
        
    	btree_for_each_safe64(&grp->bt_pic, key, node)
    	{
    		btree_remove64(&grp->bt_pic, key);
    		free(node);
    	}
    	btree_destroy64(&grp->bt_pic);
    }
    
    return 0;
}

int grp_disk_open(gfs_disk_t *disk)
{
    if(!strlen(disk->mnt_dir))
    {
        return GFS_ER_PARM;
    }
    
    if(strlen(disk->dev_name))
    {
        umount2(disk->mnt_dir, MNT_FORCE);
        if(mount(disk->dev_name, disk->mnt_dir, DISK_FS_TYPE, MS_NOATIME, NULL) < 0)
        {
            return GFS_ER_MOUNT;
        }
    }

    char av_idx[GFS_FILE_NAME_SIZE] = {0};
    grp_disk_t *gd = (grp_disk_t*)calloc(1, sizeof(grp_disk_t));

    gd->disk = *disk;

    INIT_LIST_HEAD(&gd->uvsegs);
    INIT_LIST_HEAD(&gd->upsegs);
    REC_NAME_AV_IDX(disk->mnt_dir, av_idx);
    gd->fd = open(av_idx, O_RDWR | O_CREAT, 0766);
    grp_t *grp = gd->grp = &grps[disk->grp_id];
    
    printf("open disk_id:%d, mnt_dir:%s\n", gd->disk.disk_id, gd->disk.mnt_dir);
    
    //add disks;
    int add_flag = 0;
    grp_disk_t *n;
    
    list_for_each_entry(n, &grp->disks, list)
    {
        if(gd->disk.disk_id <= n->disk.disk_id)
        {
            list_add_tail(&gd->list, &n->list);
            add_flag = 1;
        }
    }
    if(!add_flag)
    {
        list_add_tail(&gd->list, &grp->disks);
    }
    
    return 0;
}

int grp_disk_close(grp_disk_t *gd)
{
    if(gd)
    {
        printf("close disk_id:%d, mnt_dir:%s\n", gd->disk.disk_id, gd->disk.mnt_dir);
        
        grp_uvseg_t *v, *vv;
        list_for_each_entry_safe(v, vv, &gd->uvsegs, list)
    	{
    	    u64 key = v->uvseg.vseg.ch;
            key = left64(key, 32);
            key += v->uvseg.vseg.btime;
    	    
    		btree_remove64(&gd->grp->bt_av, key);
    		list_del(&v->list);
    		free(v);
    	}
    	
        grp_upseg_t *p, *pp;
        list_for_each_entry_safe(p, pp, &gd->upsegs, list)
    	{
    	    u64 key = p->upseg.pseg.ms5;
            key = left64(key, 8);
            key += p->upseg.pseg.ch;
            key = left64(key, 32);
            key += p->upseg.pseg.btime;
    	    
    		btree_remove64(&gd->grp->bt_pic, key);
    		list_del(&p->list);
    		free(p);
    	}
    	
    	list_del(&gd->list);
        close(gd->fd);
    	free(gd->av_hdr);
        if(strlen(gd->disk.dev_name))
        {
            umount2(gd->disk.mnt_dir, MNT_FORCE);
        }
    	free(gd);
    }
    
    return 0;
}

static int grp_idx_bt(grp_disk_t *gd, int b, int e)
{
    int i = 0, j = 0;
    
    printf("bt disk_id: %d, b: %d, e: %d\n", gd->disk.disk_id, b, e);
    
    gfs_bkt_t *bkt = (gfs_bkt_t *)malloc(GFS_IDX_BKT_SIZE);
    
    for(i = b; i < e; i++)
    {
        idx_bkt_seg_read(gd->fd, i, bkt);
        //printf("bt --- i: %d, seg_cnt: %d\n", i, bkt->seg_cnt);
        if(bkt->fpic)
        {
            for(j = 0; j < bkt->seg_cnt; j++)
            {
                gfs_pseg_t *pseg = (gfs_pseg_t*)(bkt+1);
                grp_useg_sync(gd, i, bkt->fpic, &pseg[j]);
            }
        }
        else
        {
            for(j = 0; j < bkt->seg_cnt; j++)
            {
                gfs_vseg_t *vseg = (gfs_vseg_t*)(bkt+1);
                grp_useg_sync(gd, i, bkt->fpic, &vseg[j]);
                //printf("bt ------ vseg: %d, dat_beg:%d dat_end:%d\n", j, vseg[j].dat_beg, vseg[j].dat_end);
            }
        }
    }
    
    free(bkt);
    
    return 0;
}


static inline int hdr_reset(grp_disk_t *n)
{
    int i = 0;
    n->av_hdr->used_cnt++;
    n->av_hdr->bkt_next = 0;
    for(i = 0; i < GFS_CH_MAX_NUM; i++)
    {
        n->av_hdr->bkt_curr[i] = GFS_INVALID_VAL;
    }
    idx_hdr_write(n->fd, n->av_hdr);
    return 0;
}


int grp_idx_load(int grp_id)
{
    int i = 0;
    grp_t *grp = &grps[grp_id];
    
    if(grp->stat >= GRP_STAT_LOAD)
    {
        return GFS_ER_ALOAD;
    }
    grp->stat = GRP_STAT_LOAD;
    
    grp_disk_t *n;
    
    printf("\n###### grp_id: %d ######\n", grp_id);
    list_for_each_entry(n, &grp->disks, list)
    {
        n->av_hdr = (gfs_hdr_t*)malloc(GFS_IDX_HDR_SIZE);
        idx_hdr_read(n->fd, n->av_hdr);
        
        printf("disk_id: %d\n", n->disk.disk_id);

        printf("used_cnt:%d\n", n->av_hdr->used_cnt);
        printf("bkt_total:%d\n", n->av_hdr->bkt_total);
        printf("bkt_size:%d\n", n->av_hdr->bkt_size);
        printf("bkt_next:%d\n", n->av_hdr->bkt_next);
        printf("bkt_curr:\n---------\n");
        char str[2048] = {0};
        char token[16];
        for(i = 0; i < GFS_CH_MAX_NUM; i++)
        {
            if(i%16 == 0) {sprintf(token, "\n%04d: ", i); strcat(str, token);}
            sprintf(token, "%06d ", n->av_hdr->bkt_curr[i]); strcat(str, token);
        }
        printf("%s\n", str);
        printf("\n---------\n");
    }
    printf("\n###### END #####\n\n");

    for(i = 0; i < 2; i++)
    {
        unsigned int min_used_cnt = 0xFFFFFFFF;
        list_for_each_entry(n, &grp->disks, list)
        {
            if(i == 0)
            {
                if(!grp->curr)
                {
                    if(n->av_hdr->bkt_next < n->av_hdr->bkt_total)
                    {
                        grp->curr = n;
                        break;
                    }   
                }
            }
            else
            {
                if(n->av_hdr->used_cnt < min_used_cnt)
                {
                    grp->curr = n;
                    min_used_cnt = n->av_hdr->used_cnt;
                }
            }
        }
        
        if(grp->curr)
        {
            n = grp->curr;
            printf("curr disk_id: %d\n", n->disk.disk_id);
            if(n->av_hdr->bkt_next >= n->av_hdr->bkt_total)
            {
                hdr_reset(n);
            }
            break;
        }
    }
    
    if(!grp->curr)
        return GFS_ER_NDISK;

    // load curr bottom;
    n = grp->curr;
    if(n->av_hdr->used_cnt != 0)
    {
        grp_idx_bt(n, n->av_hdr->bkt_next, n->av_hdr->bkt_total);
    }
    
    struct list_head *nn = n->list.next;
    while(1)
    {
        //skip head;
        if(nn == &grp->disks)
        {
            nn = nn->next;
        }
        //next is self;
        if(nn == &grp->curr->list)
        {
            break;
        }
        
        //next;
        n = list_entry(nn, grp_disk_t, list);
        nn = n->list.next;
        
        if(n->av_hdr->bkt_next > 0)
        {
            grp_idx_bt(n, 0, n->av_hdr->bkt_total);
        }
    }
    
    // load curr top;
    n = grp->curr;
    if(grp->curr)
    {
        if(n->av_hdr->bkt_next > 0)
        {
            grp_idx_bt(n, 0, n->av_hdr->bkt_next);
        }
    }
    
    return 0;
}

static inline int bkt_reset(grp_disk_t *gd, int ch, int bkt_no)
{
    int i = 0;
    gfs_bkt_t *bkt = (gfs_bkt_t *)malloc(GFS_IDX_BKT_SIZE);
    idx_bkt_seg_read(gd->fd, bkt_no, bkt);
    
    printf("NEW ch:%d, disk_id:%d, bkt_curr[%d]: %d(fpic:%d,segs:%d)\n"
        , ch
        , gd->disk.disk_id
        , ch
        , bkt_no
        , bkt->fpic
        , bkt->seg_cnt);
    
    // sync bt;
    struct btree_head64 *head;
    u64 key;
    if(bkt->fpic)
    {
        head = &gd->grp->bt_pic;
        gfs_pseg_t *pseg = (gfs_pseg_t *)(bkt+1);
        for(i = 0; i < bkt->seg_cnt; i++)
        {
            key = pseg[i].ms5;
            key = left64(key, 8);
            key += pseg[i].ch;
            key = left64(key, 32);
            key += pseg[i].btime;
            
            grp_upseg_t *n;
            if ((n = (grp_upseg_t*)btree_lookup64(head, key)) != NULL) 
        	{
              #ifdef DEBUG
            	printf("del pseg(%d) -> disk_id:%d, bkt_id:%d, ch:%d, btime:%x, ms5:%d\n"
            	    , i
            	    , n->upseg.disk_id
            	    , n->upseg.bkt_id
            	    , n->upseg.pseg.ch
            	    , n->upseg.pseg.btime
            	    , n->upseg.pseg.ms5);
                #endif
                btree_remove64(head, key);
                list_del(&n->list);
                free(n);
        	}
        	else
        	{
                #if 1
            	printf("??? pseg(%d) -> disk_id:%d, bkt_id:%d, ch:%d, btime:%x, ms5:%d\n"
            	    , i
            	    , gd->disk.disk_id
            	    , bkt_no
            	    , pseg[i].ch
            	    , pseg[i].btime
            	    , pseg[i].ms5);
                #endif
        	}
        }
    }
    else
    {
        head = &gd->grp->bt_av;
        gfs_vseg_t *vseg = (gfs_vseg_t *)(bkt+1);
        for(i = 0; i < bkt->seg_cnt; i++)
        {
            key = vseg[i].ch;
            key = left64(key, 32);
            key += vseg[i].btime;
            
            grp_uvseg_t *n;
            if ((n = (grp_uvseg_t*)btree_lookup64(head, key)) != NULL) 
        	{
                #ifdef DEBUG
            	printf("del vseg(%d) -> disk_id:%d, bkt_id:%d, ch:%d, btime:%d, etime:%d\n"
            	    , i
            	    , n->uvseg.disk_id
            	    , n->uvseg.bkt_id
            	    , n->uvseg.vseg.ch
            	    , n->uvseg.vseg.btime
            	    , n->uvseg.vseg.etime);
                #endif
                btree_remove64(head, key);
                list_del(&n->list);
                free(n);
        	}
        	else
        	{
                #if 1
            	printf("??? vseg(%d) -> disk_id:%d, bkt_id:%d, ch:%d, btime:%d, etime:%d\n"
            	    , i
            	    , gd->disk.disk_id
            	    , bkt_no
            	    , vseg[i].ch
            	    , vseg[i].btime
            	    , vseg[i].etime);
                #endif
        	}
        }
    }
    // seg_cnt = 0;
    bkt->seg_cnt = 0;
    bkt->ferr  = 0;
    bkt->flock = 0;
    bkt->fpic  = (ch == GFS_CH_PIC_NO)?1:0;
    idx_bkt_write(gd->fd, bkt_no, bkt);
    
    free(bkt);
    return 0;
}


int grp_get_curr_disk(int grp_id, int ch, grp_disk_t **gd, int *bkt_no)
{    
    grp_t *grp = &grps[grp_id];
    
    grp_disk_t *gdisk = *gd = grp->curr;
    
    if(!gdisk)
        return -1;
        
    if(gdisk->av_hdr->bkt_curr[ch] != GFS_INVALID_VAL)
    {
        *bkt_no = gdisk->av_hdr->bkt_curr[ch];
        printf("CUR ch:%d, disk_id:%d, bkt_curr[%d]: %d\n", ch, gdisk->disk.disk_id, ch, *bkt_no);
    }
    else
    {
        if(gdisk->av_hdr->bkt_next >= gdisk->av_hdr->bkt_total)
        {
            struct list_head *nn = grp->curr->list.next;
            //skip head;
            if(nn == &grp->disks)
            {
                nn = nn->next;
            }
            gdisk = *gd = grp->curr = list_entry(nn, grp_disk_t, list);
            if(gdisk->av_hdr->bkt_next >= gdisk->av_hdr->bkt_total)
            {
                hdr_reset(gdisk);
            }
        }
        
        if(gdisk->av_hdr->bkt_next < gdisk->av_hdr->bkt_total)
        {
            *bkt_no = gdisk->av_hdr->bkt_curr[ch] = gdisk->av_hdr->bkt_next;
            gdisk->av_hdr->bkt_next++;

            bkt_reset(gdisk, ch, *bkt_no);
            idx_hdr_write(gdisk->fd, gdisk->av_hdr);
        }
    }
    
    return 0;
}

int grp_get_disk(int grp_id, int disk_id, grp_disk_t **gd)
{
    grp_t *grp = &grps[grp_id];
    grp_disk_t *n;
    
    list_for_each_entry(n, &grp->disks, list)
    {
        if(n->disk.disk_id == disk_id)
        {
            *gd = n;
            return 0;
        }
    }
    return GFS_ER_NDISK;
}

int grp_rel_disk(grp_disk_t *gd)
{
    if(gd)
    {
        ;
    }
    return 0;
}

int grp_disk_each(int grp_id, gfs_gdisk_q_t *q)
{
    grp_t *grp = &grps[grp_id];
    grp_disk_t *n;
    
    list_for_each_entry(n, &grp->disks, list)
    {
        q->cb(q, n->av_hdr);
    }
    return 0;
}


int grp_useg_query(gfs_rec_q_t *q)
{
    grp_t *grp = &grps[q->grp_id];
    struct btree_head64 *head;
    
    u64 key = q->etime;
    
    if(q->fpic)
    {
        head = &grp->bt_pic;
        grp_upseg_t *n = (grp_upseg_t*)btree_lookup_le64(head, key);
        if(n == NULL)
        {
            return 0;
        }
        
        if(n->upseg.pseg.btime < q->btime)
        {
            return 0;
        }
        
        if(q->cb(q, &n->upseg))
            return 0;
        
        grp_upseg_t *nn;
        while((nn = (grp_upseg_t*)btree_get_prev64(head, &key)) != NULL)
        {            
            if(nn->upseg.pseg.btime < q->btime)
            {
                break;
            }
            if(nn != n)
            {
                if(q->cb(q, &nn->upseg))
                    break;
            }
        }
    }
    else
    {
        head = &grp->bt_av;
        grp_uvseg_t *n = (grp_uvseg_t*)btree_lookup_le64(head, key);
        if(n == NULL)
        {
            return 0;
        }
        
        if(n->uvseg.vseg.etime < q->btime)
        {
            return 0;
        }
        
        if(q->cb(q, &n->uvseg))
            return 0;
                
        grp_uvseg_t *nn;
        while((nn = (grp_uvseg_t*)btree_get_prev64(head, &key)) != NULL)
        {    	    
            if(nn->uvseg.vseg.etime < q->btime)
            {
                break;
            }
            if(nn != n)
            {
                if(q->cb(q, &nn->uvseg))
                    break;
            }
        }
    }
        
    return 0;
}
int grp_useg_sync(grp_disk_t *gd, int bkt_no, int fpic, void *seg)
{
    struct btree_head64 *head;
    u64 key;
    if(fpic)
    {
        head = &gd->grp->bt_pic;
        gfs_pseg_t *pseg = (gfs_pseg_t*)seg;
        
        key = pseg->ms5;
        key = left64(key, 8);
        key += pseg->ch;
        key = left64(key, 32);
        key += pseg->btime;
        
        grp_upseg_t *n;
        if ((n = (grp_upseg_t*)btree_lookup64(head, key)) != NULL) 
    	{
    	    list_del(&n->list);
    	    n->upseg.disk_id = gd->disk.disk_id;
    	    n->upseg.bkt_id = bkt_no;
    	    n->upseg.pseg = *pseg;
    	    list_add(&n->list, &gd->upsegs);
    		btree_update64(head, key, n);

#if 1
    	printf("warn -> disk_id:%d, bkt_id:%d, ch:%d, btime:%x, ms5:%d\n"
    	    , n->upseg.disk_id
    	    , n->upseg.bkt_id
    	    , n->upseg.pseg.ch
    	    , n->upseg.pseg.btime
    	    , n->upseg.pseg.ms5);
#endif

    	}
    	else
    	{
    	    n = (grp_upseg_t*)calloc(1, sizeof(grp_upseg_t));
    	    n->upseg.disk_id = gd->disk.disk_id;
    	    n->upseg.bkt_id = bkt_no;
    	    n->upseg.pseg = *pseg;
    	    list_add(&n->list, &gd->upsegs);
    		btree_insert64(head, key, n, 0);
    	}
    	
#ifdef DEBUG
    	printf("pseg-> disk_id:%d, bkt_id:%d, ch:%d, btime:%x, ms5:%d\n"
    	    , n->upseg.disk_id
    	    , n->upseg.bkt_id
    	    , n->upseg.pseg.ch
    	    , n->upseg.pseg.btime
    	    , n->upseg.pseg.ms5);
#endif
    	
    }
    else
    {
        head = &gd->grp->bt_av;
        gfs_vseg_t *vseg = (gfs_vseg_t*)seg;
        
        key = vseg->ch;
        key = left64(key, 32);
        key += vseg->btime;
        
        grp_uvseg_t *n;
        if ((n = (grp_uvseg_t*)btree_lookup64(head, key)) != NULL) 
    	{
    	    list_del(&n->list);
    	    n->uvseg.disk_id = gd->disk.disk_id;
    	    n->uvseg.bkt_id = bkt_no;
    	    n->uvseg.vseg = *vseg;
    	    list_add(&n->list, &gd->uvsegs);
    		btree_update64(head, key, n);
#if 1
        if(n->uvseg.vseg.ch != vseg->ch)
    	printf("warn -> disk_id:%d, bkt_id:%d, ch:%d, btime:%d, etime:%d\n"
    	    , n->uvseg.disk_id
    	    , n->uvseg.bkt_id
    	    , n->uvseg.vseg.ch
    	    , n->uvseg.vseg.btime
    	    , n->uvseg.vseg.etime);
#endif
    	}
    	else
    	{
    	    n = (grp_uvseg_t*)calloc(1, sizeof(grp_uvseg_t));
    	    n->uvseg.disk_id = gd->disk.disk_id;
    	    n->uvseg.bkt_id = bkt_no;
    	    n->uvseg.vseg = *vseg;
    	    list_add(&n->list, &gd->uvsegs);
    		btree_insert64(head, key, n, 0);
    	}
#if 0//def DEBUG
    	printf("vseg-> disk_id:%d, bkt_id:%d, ch:%d, btime:%d, etime:%d\n"
    	    , n->uvseg.disk_id
    	    , n->uvseg.bkt_id
    	    , n->uvseg.vseg.ch
    	    , n->uvseg.vseg.btime
    	    , n->uvseg.vseg.etime);
#endif
    }
    
    return 0;
}

