#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#undef isnan
#define isnan(x) \
     __extension__({ __typeof (x) __x_a = (x); \
     __builtin_expect(__x_a != __x_a, 0); })

#undef isfinite
#define isfinite(x) \
     __extension__ ({ __typeof (x) __x_f = (x); \
     __builtin_expect(!isnan(__x_f - __x_f), 1); })

#undef isinf
#define isinf(x) \
     __extension__ ({ __typeof (x) __x_i = (x); \
     __builtin_expect(!isnan(__x_i) && !isfinite(__x_i), 0); })

#if defined(__GNUC__)
#   define    HUGE_VAL     __builtin_huge_val()
#   define    HUGE_VALF    __builtin_huge_valf()
#   define    HUGE_VALL    __builtin_huge_vall()
#   define    NAN          __builtin_nanf("0x7fc00000")
#else
#   define    HUGE_VAL     1e500
#   define    HUGE_VALF    1e50f
#   define    HUGE_VALL    1e5000L
#   define    NAN          __nan()
#endif

int string2ld(const char *s, size_t slen, long double *dp) {
    char buf[256];
    long double value;
    char *eptr;

    if (slen >= sizeof(buf)) return 0;
    memcpy(buf,s,slen);
    buf[slen] = '\0';

    errno = 0;
    value = strtold(buf, &eptr);
    if (isspace(buf[0]) || eptr[0] != '\0' ||
        (errno == ERANGE &&
            (value == HUGE_VAL || value == -HUGE_VAL || value == 0)) ||
        errno == EINVAL ||
        isnan(value))
        return 0;

    if (dp) *dp = value;

    return 1;
}

int ld2string(char *buf, size_t len, long double value, int humanfriendly) {
    size_t l;

    /* 数值太大 */
    if (isinf(value)) {
        /* Libc in odd systems (Hi Solaris!) will format infinite in a
         * different way, so better to handle it in an explicit way. */
        if (len < 5) return 0; /* No room. 5 is "-inf\0" */
        if (value > 0) {
            memcpy(buf,"inf",3);
            l = 3;
        } else {
            memcpy(buf,"-inf",4);
            l = 4;
        }
    } else if (humanfriendly) {
        /* We use 17 digits precision since with 128 bit floats that precision
         * after rounding is able to represent most small decimal numbers in a
         * way that is "non surprising" for the user (that is, most small
         * decimal numbers will be represented in a way that when converted
         * back into a string are exactly the same as what the user typed.) */
	/*
	 * 使用17位精度保存数据
	 * 128位表示的浮点数，在四舍五入的大部分小数
	 */
        l = snprintf(buf,len,"%.17Lf", value);
        if (l+1 > len) return 0; /* No room. */
        /* Now remove trailing zeroes after the '.' */
        if (strchr(buf,'.') != NULL) {
            char *p = buf+l-1;
            while(*p == '0') {
                p--;
                l--;
            }
            if (*p == '.') l--;
        }
    } else {
        l = snprintf(buf,len,"%.17Lg", value);
	/* l+1大于buf长度，没有足够空间存储 */
        if (l+1 > len) return 0; /* No room. */
    }
    /* 增加结束位 */
    buf[l] = '\0';
    return l;
}

/*
 * 可以无损进行 浮点数<=>字符串 的转换
 */
int main()
{
	long double ld = 1.12345678912334455666;
	char buf[256];
	int l = ld2string(buf, sizeof(buf), ld, 0);

	long double *ld2 = (long double *)malloc(l * sizeof(long double *));
	printf("str: %s\n", buf);/* str: 1.1234567891233445 */

	int l2 = string2ld(buf, strlen(buf), ld2);

	printf("orig long double : %.17Lg\n", *ld2);/* orig long double : 1.1234567891233445 */
	return 0;
}
