#ifndef __printd_h__
#define __printd_h__

#define LOG_LEVEL_DEBUG      0
#define LOG_LEVEL_WARN       1
#define LOG_LEVEL_ERR        2

int   gfs_setlog(int level, int (*log)(char *str));
int 	gfs_print(int level, const char *fName, const char *func, int nLine, const char *fmt, ...);

#define printf(FMT...) gfs_print(LOG_LEVEL_DEBUG,  __FILE__, __FUNCTION__, __LINE__, ##FMT)
#define printw(FMT...) gfs_print(LOG_LEVEL_WARN,   __FILE__, __FUNCTION__, __LINE__, ##FMT)
#define printe(FMT...) gfs_print(LOG_LEVEL_ERR,    __FILE__, __FUNCTION__, __LINE__, ##FMT)

#endif // __printd_h__