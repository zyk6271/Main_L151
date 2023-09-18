#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

/**
 * rt_sscanf reads formatted input from a string (only support %d %x).
 * @param  str This is the C string that the function processes as its source to retrieve the data.
 * @param  fmt This is the C string that contains one or more of the following items: Whitespace character,
 *         Non-whitespace character and Format specifiers
 * @return On success, the function returns the number of variables filled.
 *         In the case of an input failure before any data could be successfully read, EOF is returned.
 */
int rt_sscanf(const char *str, const char *fmt, ...)
{
    int ret = 0;
    const char *rp = str;
    const char *fp = fmt;
    va_list ap;
    char *ep;
    char fc;
    long v;
    int *ip;

    va_start(ap, fmt);

    while(*rp && *fp)
    {
        fc = *fp;
        if(isspace((int)fc))
        {
          /* do nothing */
        }
        else if(fc != '%')
        {
            while(isspace((int)*rp)) rp++;
            if(*rp == 0)
            {
                break;
            }
            else if(fc != *rp)
            {
                break;
            }
            else
            {
                rp++;
            }
        }
        else
        {  /* fc == '%' */
            fc = *++fp;
            if(fc == 'd' || fc == 'x')
            {
                ip = va_arg(ap, int *);
                v = strtol(rp, &ep, fc == 'd' ? 10 : 16);
                if(rp == ep)
                    break;
                rp = ep;
                *ip = v;
                ret++;
            }
            else if(fc == 's')
            {
                /* TODO */
            }
        }
        fp++;
    }

    va_end(ap);

    return ret;
}
