#ifndef __gfs_disk_h__
#define __gfs_disk_h__

/* gfs ´ÅÅÌ²Ù×÷ 
 * qq: 25088970 maohw
 */
#include "gfs.h"

#define DISK_FS_TYPE "ext4"
#define DISK_SCAN_MNT_DIR "/tmp/scan_mnt"
#define DISK_FORMAT_MNT_DIR "/tmp/format_mnt"

int disk_init(void);
int disk_uninit(void);

int disk_scan(gfs_disk_q_t *q);
int disk_format(gsf_disk_f_t *f);


#endif //~__gfs_disk_h__
