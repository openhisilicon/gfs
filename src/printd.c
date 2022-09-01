#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "printd.h"

#define COLOR_NONE           "\033[0000m"
#define COLOR_GREEN          "\033[0;32m"
#define COLOR_YELLOW         "\033[1;33m"
#define COLOR_RED            "\033[0;31m"

static int __level = 9;
static int (*__log)(char *str) = NULL;

int gfs_setlog(int level, int (*log)(char *str))
{
  __level = level;
  __log = log;
  return 0;
}

int gfs_print(int level, const char *fname, const char *func, int fline, const char *fmt, ...)
{
	va_list args;
	char str[2048] = {0};

  if(level > __level)
    return 0;

	sprintf(str, "[%d][%s:%d][%s]:", level, fname, fline, func);
	va_start(args, fmt);
	vsnprintf(&str[strlen(str)], sizeof(str) - strlen(str) - 1, fmt, args);
	va_end(args);
	
	char *COLOR_BEG = (level == LOG_LEVEL_DEBUG)?COLOR_NONE:
                    (level == LOG_LEVEL_WARN)?COLOR_YELLOW:
                    COLOR_RED;
	
  if(__log)
    __log(str);
  else
    fprintf(stderr, "%s" "%s"COLOR_NONE, COLOR_BEG, str);
    
	return 0;
}