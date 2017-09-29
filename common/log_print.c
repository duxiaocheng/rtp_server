/*****************************************************************************************/
/**
@brief log related functions
*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/
#include "log_print.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

static const char *nowtime();

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

static const char *nowtime()
{
	static char s_ctime[64] = {0};
  struct timeval tv = {0};
	struct tm t = {0};
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &t);

	snprintf(s_ctime, sizeof(s_ctime) - 1, "%02d:%02d:%02d.%03d",
		t.tm_hour, t.tm_min, t.tm_sec, (int)(tv.tv_usec / 1000));
  return s_ctime;
}

/*****************************************************************************************/
int log_print(int log_level, const char *fmt, ...)
{
  int ret = -1;
  static const char* pre[] = {"\0", "fatal", "error", "warn", "info", "trace", "debug"};
	if (log_level >= 1 && log_level <= 6) {
    printf("%s [%s] %s: ", nowtime(), "rtps", pre[log_level]);
		va_list ap;
    va_start(ap, fmt);
    ret = vprintf(fmt, ap); // print to stdout
    va_end(ap);
	}
  return ret;
}

void hexdump(const void* buff, unsigned int len)
{
    unsigned int i = 0;
    unsigned int j = 0;
    const unsigned char *pdata = (const unsigned char *)buff;
    const int max_ch_of_line = 16;
    for (i = 0; i < len; i += max_ch_of_line) {
        char buf_0x[128] = {0};
        char buf_ch[128] = {0};
        for (j = 0; j < max_ch_of_line; j++) {
            snprintf(buf_0x + strlen(buf_0x), sizeof(buf_0x) - strlen(buf_0x) - 1, 
                "%02x ", (*pdata & 0xff)); /* avoid 0xffffff09 */
            snprintf(buf_ch + strlen(buf_ch), sizeof(buf_ch) - strlen(buf_ch) - 1, 
                "%c", *pdata > 31 && *pdata <= 'z' ? *pdata : '.');
            pdata++;
        }
        debugf("0x%p: %s %s\n", (pdata - 16), buf_0x, buf_ch);
        if (0 != i && i % 512 == 0) {
            debugf("\n");
        }
    }
}

