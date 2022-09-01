#ifndef __gfs_stru_h__
#define __gfs_stru_h__

/* gfs 结构体定义: 磁盘4k对齐, 内存4字节对齐 
 * qq: 25088970 maohw
 */

/*---------------------------------------------------------------|
|----------------------- group0 ---------------------------------|
|-------|    |-------|    |-------|                 btree        |
| disk0 |--->| disk1 |--->| disk2 |                  /\          |
|-------|    |-------|    |-------|                 /  \         |
|                                                  n    n        |
|--- av.idx --- |                                 /\   / \       |
| 4k  gfs_hdr_t |----| bkt_next                  n  n  n  n      |
| 16k gfs_bkt_t |    v                           |  |  |   |     |
| 16k ........  |<------------------------------ |  |  |   |     |
| 16k ........  |<----------------------------------|  |   |     |
| 16k ........  |<-------------------------------------|   |     |
|---------------|<-----------------------------------------|     |
|                                                                |
|----- av.dat -----|                                             |
| 32k  gfs_idx_t   |                                             |
| ...  ............|                                             |
| ...  ............|                                             |
| ...  ............|                                             |
|------------------|                                             |
|----------------------------------------------------------------*/


#define GFS_GRP_MAX_NUM     (4)     // 最大盘组个数
#define GFS_CH_MAX_NUM      (128)   // 最大通道个数
#define GFS_CH_PIC_NO       (GFS_CH_MAX_NUM-1) // 图片通道号
#define GFS_DEV_NAME_SIZE   (128)   // 最大设备名长度
#define GFS_FILE_NAME_SIZE  (128)   // 最大文件名长度

#define GFS_IDX_HDR_SIZE    (4*1024)    // 索引文件头长度
#define GFS_IDX_BKT_SIZE    (16*1024)   // 文件信息长度
#define GFS_IDX_FILE_SIZE(n) (GFS_IDX_HDR_SIZE+(n)*GFS_IDX_BKT_SIZE) // 索引文件大小

#define GFS_FILE_SIZE       (256*1024*1024) // 数据文件大小
#define GFS_FILE_IDX_NUM    (4096)          // 数据索引个数
#define GFS_FILE_IDX_SIZE   (GFS_FILE_IDX_NUM*sizeof(gfs_idx_t)) // 数据索引大小

#define GFS_VSEG_MAX_NUM     ((GFS_IDX_BKT_SIZE-sizeof(gfs_bkt_t))/sizeof(gfs_vseg_t)) // 单文件最大录像片段
#define GFS_PSEG_MAX_NUM    ((GFS_IDX_BKT_SIZE-sizeof(gfs_bkt_t))/sizeof(gfs_pseg_t))  // 单文件最大图片片段

#define GFS_INVALID_VAL     (~0)                // 无效值
#define GFS_ALIGN(val,align) (((val) + (align)-1) & ~((align)-1)) // 对齐
#define GFS_ALIGN_4K(val)   GFS_ALIGN(val,4096) // 4K对齐
#define GFS_ROUND(val,align) ((val)/(align)*(align)) // 取整
#define GFS_ROUND_4K(val)   GFS_ROUND(val,4096) // 4K取整
#define GFS_REC_BUF_SIZE    (1*1024*1024)       // 录像缓存大小
#define GFS_REC_MAX_SIZE    (3*GFS_REC_BUF_SIZE)// 最大录像帧长度
#define GFS_PIC_MAX_SIZE    (3*1024*1024)       // 最大图片帧长度


typedef struct gfs_cfg_s    // 8byte
{
    int grp_id;             // 盘组ID [0 - GFS_GRP_MAX_NUM]
    int disk_id;            // 磁盘ID [1 - ...]
}gfs_cfg_t;


typedef struct  gfs_hdr_s       // 4k
{
    unsigned int used_cnt;      // 磁盘使用次数
    unsigned int bkt_total;     // 文件个数
    unsigned int bkt_size;      // 文件大小
    unsigned int bkt_next;      // 下一个可用的文件序号
    unsigned int bkt_curr[0];   // 当前正在使用的文件序号
}gfs_hdr_t;

typedef struct gfs_vseg_s   // 32 byte
{
    unsigned int btime;     // 片段开始时间
    unsigned int etime;     // 片段结束时间
    unsigned int dat_beg;   // 片段数据开始位置
    unsigned int dat_end;   // 片段数据结束位置
    unsigned int idx_beg;   // 片段索引开始位置
    unsigned int idx_end;   // 片段索引结束位置
    unsigned char ch;       // 片段所属通道号
    unsigned char fmt;      // 片段录像数据格式
    unsigned short tags;    // 片段录像类型标签
    unsigned char res[4];   // 保留
}gfs_vseg_t;

typedef struct gfs_pseg_s   // 16 byte
{
    unsigned int btime;     // 片段开始时间
    unsigned int dat_beg;   // 片段数据开始位置
    unsigned int dat_end;   // 片段数据结束位置
    unsigned char ch;       // 片段所属通道号
    unsigned char ms5;      // 片段开始时间扩充 精度 5ms
    unsigned short tags;    // 片段录像类型标签
}gfs_pseg_t;


typedef struct gfs_bkt_s     // 32 byte
{
    unsigned int seg_cnt;    // 片段个数
    unsigned char ferr;      // 错误标示
    unsigned char flock;     // 锁定标示
    unsigned char fpic;      // 图片标示
    unsigned char r1[1];     // -----
    unsigned int res[6];     // 保留
}gfs_bkt_t;


typedef struct gfs_idx_s    // 8 byte
{
    unsigned int time;      //时间
    unsigned int off;       //位置
}gfs_idx_t;


#endif //~__gfs_stru_h__