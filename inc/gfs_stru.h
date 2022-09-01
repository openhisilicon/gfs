#ifndef __gfs_stru_h__
#define __gfs_stru_h__

/* gfs �ṹ�嶨��: ����4k����, �ڴ�4�ֽڶ��� 
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


#define GFS_GRP_MAX_NUM     (4)     // ����������
#define GFS_CH_MAX_NUM      (128)   // ���ͨ������
#define GFS_CH_PIC_NO       (GFS_CH_MAX_NUM-1) // ͼƬͨ����
#define GFS_DEV_NAME_SIZE   (128)   // ����豸������
#define GFS_FILE_NAME_SIZE  (128)   // ����ļ�������

#define GFS_IDX_HDR_SIZE    (4*1024)    // �����ļ�ͷ����
#define GFS_IDX_BKT_SIZE    (16*1024)   // �ļ���Ϣ����
#define GFS_IDX_FILE_SIZE(n) (GFS_IDX_HDR_SIZE+(n)*GFS_IDX_BKT_SIZE) // �����ļ���С

#define GFS_FILE_SIZE       (256*1024*1024) // �����ļ���С
#define GFS_FILE_IDX_NUM    (4096)          // ������������
#define GFS_FILE_IDX_SIZE   (GFS_FILE_IDX_NUM*sizeof(gfs_idx_t)) // ����������С

#define GFS_VSEG_MAX_NUM     ((GFS_IDX_BKT_SIZE-sizeof(gfs_bkt_t))/sizeof(gfs_vseg_t)) // ���ļ����¼��Ƭ��
#define GFS_PSEG_MAX_NUM    ((GFS_IDX_BKT_SIZE-sizeof(gfs_bkt_t))/sizeof(gfs_pseg_t))  // ���ļ����ͼƬƬ��

#define GFS_INVALID_VAL     (~0)                // ��Чֵ
#define GFS_ALIGN(val,align) (((val) + (align)-1) & ~((align)-1)) // ����
#define GFS_ALIGN_4K(val)   GFS_ALIGN(val,4096) // 4K����
#define GFS_ROUND(val,align) ((val)/(align)*(align)) // ȡ��
#define GFS_ROUND_4K(val)   GFS_ROUND(val,4096) // 4Kȡ��
#define GFS_REC_BUF_SIZE    (1*1024*1024)       // ¼�񻺴��С
#define GFS_REC_MAX_SIZE    (3*GFS_REC_BUF_SIZE)// ���¼��֡����
#define GFS_PIC_MAX_SIZE    (3*1024*1024)       // ���ͼƬ֡����


typedef struct gfs_cfg_s    // 8byte
{
    int grp_id;             // ����ID [0 - GFS_GRP_MAX_NUM]
    int disk_id;            // ����ID [1 - ...]
}gfs_cfg_t;


typedef struct  gfs_hdr_s       // 4k
{
    unsigned int used_cnt;      // ����ʹ�ô���
    unsigned int bkt_total;     // �ļ�����
    unsigned int bkt_size;      // �ļ���С
    unsigned int bkt_next;      // ��һ�����õ��ļ����
    unsigned int bkt_curr[0];   // ��ǰ����ʹ�õ��ļ����
}gfs_hdr_t;

typedef struct gfs_vseg_s   // 32 byte
{
    unsigned int btime;     // Ƭ�ο�ʼʱ��
    unsigned int etime;     // Ƭ�ν���ʱ��
    unsigned int dat_beg;   // Ƭ�����ݿ�ʼλ��
    unsigned int dat_end;   // Ƭ�����ݽ���λ��
    unsigned int idx_beg;   // Ƭ��������ʼλ��
    unsigned int idx_end;   // Ƭ����������λ��
    unsigned char ch;       // Ƭ������ͨ����
    unsigned char fmt;      // Ƭ��¼�����ݸ�ʽ
    unsigned short tags;    // Ƭ��¼�����ͱ�ǩ
    unsigned char res[4];   // ����
}gfs_vseg_t;

typedef struct gfs_pseg_s   // 16 byte
{
    unsigned int btime;     // Ƭ�ο�ʼʱ��
    unsigned int dat_beg;   // Ƭ�����ݿ�ʼλ��
    unsigned int dat_end;   // Ƭ�����ݽ���λ��
    unsigned char ch;       // Ƭ������ͨ����
    unsigned char ms5;      // Ƭ�ο�ʼʱ������ ���� 5ms
    unsigned short tags;    // Ƭ��¼�����ͱ�ǩ
}gfs_pseg_t;


typedef struct gfs_bkt_s     // 32 byte
{
    unsigned int seg_cnt;    // Ƭ�θ���
    unsigned char ferr;      // �����ʾ
    unsigned char flock;     // ������ʾ
    unsigned char fpic;      // ͼƬ��ʾ
    unsigned char r1[1];     // -----
    unsigned int res[6];     // ����
}gfs_bkt_t;


typedef struct gfs_idx_s    // 8 byte
{
    unsigned int time;      //ʱ��
    unsigned int off;       //λ��
}gfs_idx_t;


#endif //~__gfs_stru_h__