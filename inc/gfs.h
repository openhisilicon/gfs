#ifndef __gfs_h__
#define __gfs_h__

#ifdef __cplusplus
extern "C" {
#endif


/* gfs 接口定义 
 * qq: 25088970 maohw
 */

#include "gfs_stru.h"

enum {
    GFS_ER_OK       =  0,       // 成功
    GFS_ER_ERR      = -1,       // 失败
    GFS_ER_INIT     = -2,       // 未初始化
    GFS_ER_PARM     = -3,       // 参数错误
    GFS_ER_NDISK    = -4,       // 磁盘未找到
    GFS_ER_MOUNT    = -5,       // 磁盘挂载错误
    GFS_ER_EDISK    = -6,       // 磁盘读写错误
    GFS_ER_ADISK    = -7,       // 磁盘已添加
    GFS_ER_ALOAD    = -8,       // 盘组已装载
    GFS_ER_DEOF     = -9,       // 文件数据EOF
    GFS_ER_IEOF     = -10,      // 文件索引EOF
    GFS_ER_SEOF     = -11,      // 文件片段EOF
    GFS_ER_BIGFRM   = -12,      // 帧长度太大
};

typedef struct gfs_init_parm_s
{
    int (*err)(int er);
    int (*log)(char *str);
}gfs_init_parm_t;

int gfs_init(gfs_init_parm_t *parm);        // 初始化
int gfs_uninit(void);                       // 反初始化


typedef struct gfs_disk_s 
{
    int  grp_id;
    int  disk_id;
    unsigned int size; // M
    char dev_name[GFS_DEV_NAME_SIZE];
    char mnt_dir[GFS_DEV_NAME_SIZE];
}gfs_disk_t;

typedef struct gfs_disk_q_s
{
    void* uargs;
    int (*cb)(struct gfs_disk_q_s *owner, gfs_disk_t *disk);
}gfs_disk_q_t;

typedef struct gsf_disk_f_s
{
    gfs_disk_t *disk;
    void *uargs;
    int (*cb)(struct gsf_disk_f_s *owner, int percent);
}gsf_disk_f_t;

int gfs_disk_scan(gfs_disk_q_t *q);     // 磁盘扫描;
int gfs_disk_format(gsf_disk_f_t *f);   // 格式化磁盘;


typedef struct gfs_gdisk_q_s
{
    void* uargs;
    int (*cb)(struct gfs_gdisk_q_s *owner, gfs_hdr_t *hdr);
}gfs_gdisk_q_t;

int gfs_grp_disk_add(gfs_disk_t *disk);             // 添加磁盘到盘组
int gfs_grp_disk_del(int grp_id, int disk_id);      // 删除磁盘从盘组
int gfs_grp_disk_each(int grp_id, gfs_gdisk_q_t *q);// 遍历磁盘从盘组
int gsf_grp_idx_load(int grp_id);                   // 装载盘组

typedef struct gfs_uvseg_s
{
    int disk_id;
    int bkt_id;
    gfs_vseg_t vseg;
}gfs_uvseg_t;

typedef struct gfs_upseg_s
{
    int disk_id;
    int bkt_id;
    gfs_pseg_t pseg;
}gfs_upseg_t;

typedef struct gfs_rec_q_s
{
    int fpic;
    int grp_id;
    int btime;
    int etime;
    void* uargs;
    int (*cb)(struct gfs_rec_q_s *owner, void *useg);
}gfs_rec_q_t;

typedef struct gfs_rec_w_s
{
    int ch;
    int fmt;
}gfs_rec_w_t;


typedef struct gfs_rec_rw_s {
    int grp_id;
    int fpic;
    int frw;
    union {
        gfs_uvseg_t rv;
        gfs_upseg_t rp;
        gfs_rec_w_t w;
    }parm;
}gfs_rec_rw_t;

typedef struct gfs_frm_s {
    int widx;       // 视频数据时表示是否为关键帧, 图片数据时表示图片通道号;
    int time;       // 数据时间戳
    int time_ms;    // 数据时间戳扩充 ms;
    int tags;       // 数据标示,如 录像类型
    int size;       // 数据大小
    char *data;     // 数据地址
}gfs_frm_t;


int gfs_rec_query(gfs_rec_q_t *q);                  // 查询录像
int gfs_rec_open(void **hdl, gfs_rec_rw_t *rw);     // 打开录像句柄
int gfs_rec_open_r(void **hdl, gfs_rec_rw_t *rw, char *mnt); //只读打开,指定mnt[/REC/0000001F]
int gfs_rec_write(void *hdl, gfs_frm_t *frm);       // 写录像
int gfs_rec_read(void *hdl, char *buf, int *size);  // 读录像
int gfs_rec_seek_time(void *hdl, int time);         // 绝对跳转到[索引时间戳]
int gfs_rec_seek_pos(void *hdl, int pos);           // 相对跳转到[索引号偏移]
int gfs_rec_close(void *hdl);                       // 关闭录像句柄

#ifdef __cplusplus
}
#endif

#endif //~__gfs_h__