#ifndef __gfs_h__
#define __gfs_h__

#ifdef __cplusplus
extern "C" {
#endif


/* gfs �ӿڶ��� 
 * qq: 25088970 maohw
 */

#include "gfs_stru.h"

enum {
    GFS_ER_OK       =  0,       // �ɹ�
    GFS_ER_ERR      = -1,       // ʧ��
    GFS_ER_INIT     = -2,       // δ��ʼ��
    GFS_ER_PARM     = -3,       // ��������
    GFS_ER_NDISK    = -4,       // ����δ�ҵ�
    GFS_ER_MOUNT    = -5,       // ���̹��ش���
    GFS_ER_EDISK    = -6,       // ���̶�д����
    GFS_ER_ADISK    = -7,       // ���������
    GFS_ER_ALOAD    = -8,       // ������װ��
    GFS_ER_DEOF     = -9,       // �ļ�����EOF
    GFS_ER_IEOF     = -10,      // �ļ�����EOF
    GFS_ER_SEOF     = -11,      // �ļ�Ƭ��EOF
    GFS_ER_BIGFRM   = -12,      // ֡����̫��
};

typedef struct gfs_init_parm_s
{
    int (*err)(int er);
    int (*log)(char *str);
}gfs_init_parm_t;

int gfs_init(gfs_init_parm_t *parm);        // ��ʼ��
int gfs_uninit(void);                       // ����ʼ��


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

int gfs_disk_scan(gfs_disk_q_t *q);     // ����ɨ��;
int gfs_disk_format(gsf_disk_f_t *f);   // ��ʽ������;


typedef struct gfs_gdisk_q_s
{
    void* uargs;
    int (*cb)(struct gfs_gdisk_q_s *owner, gfs_hdr_t *hdr);
}gfs_gdisk_q_t;

int gfs_grp_disk_add(gfs_disk_t *disk);             // ��Ӵ��̵�����
int gfs_grp_disk_del(int grp_id, int disk_id);      // ɾ�����̴�����
int gfs_grp_disk_each(int grp_id, gfs_gdisk_q_t *q);// �������̴�����
int gsf_grp_idx_load(int grp_id);                   // װ������

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
    int widx;       // ��Ƶ����ʱ��ʾ�Ƿ�Ϊ�ؼ�֡, ͼƬ����ʱ��ʾͼƬͨ����;
    int time;       // ����ʱ���
    int time_ms;    // ����ʱ������� ms;
    int tags;       // ���ݱ�ʾ,�� ¼������
    int size;       // ���ݴ�С
    char *data;     // ���ݵ�ַ
}gfs_frm_t;


int gfs_rec_query(gfs_rec_q_t *q);                  // ��ѯ¼��
int gfs_rec_open(void **hdl, gfs_rec_rw_t *rw);     // ��¼����
int gfs_rec_open_r(void **hdl, gfs_rec_rw_t *rw, char *mnt); //ֻ����,ָ��mnt[/REC/0000001F]
int gfs_rec_write(void *hdl, gfs_frm_t *frm);       // д¼��
int gfs_rec_read(void *hdl, char *buf, int *size);  // ��¼��
int gfs_rec_seek_time(void *hdl, int time);         // ������ת��[����ʱ���]
int gfs_rec_seek_pos(void *hdl, int pos);           // �����ת��[������ƫ��]
int gfs_rec_close(void *hdl);                       // �ر�¼����

#ifdef __cplusplus
}
#endif

#endif //~__gfs_h__