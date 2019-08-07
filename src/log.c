#include "base.h"
#include <stdarg.h>

FILE *fp_err;
FILE *fp_log;

char *err_name = "err.log";
char *log_name = "screen.log";

void init_logs()
{
    if(err_name)
    {
		fp_err = fopen(err_name, "wb");
        if(!fp_err)
        {
			DIE("fopen err_name %s", err_name);
        }
    }
    if(log_name)
    {
		fp_log = fopen(log_name, "wb");
        if(!fp_log)
        {
			DIE("fopen log_name %s", log_name);
        }
    }
}

void close_logs()
{
    if(fp_err)
        fclose(fp_err);

    if(fp_log)
        fclose(fp_log);
}


void log_msg(const char *fmt, ...)
{
    char buf[2048] = {0};
    char *ptr = buf;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(ptr, fmt, ap);
    va_end(ap);

    fprintf(fp_log, "%s", buf);
    fflush(fp_log);
}

void err_msg(const char *fmt, ...)
{
    char buf[2048] = {0};
    char *ptr = buf;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(ptr, fmt, ap);
    va_end(ap);

    fprintf(fp_err, "%s", buf);
    fflush(fp_err);
}

