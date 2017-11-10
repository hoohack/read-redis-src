#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

typedef char *sds;

/* Note: sdshdr5 is never used, we just access the flags byte directly.
 *  * However is here to document the layout of type 5 SDS strings. */
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len; /* used */
    uint8_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len; /* used */
    uint32_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7
#define SDS_TYPE_BITS 3
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)

#define s_malloc malloc
#define s_realloc realloc
#define s_free free

static inline size_t sdslen(const sds s) {
    unsigned char flags = s[-1];

    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->len;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->len;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->len;
    }
    return 0;
}

static inline void sdssetlen(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len = newlen;
            break;
    }
}


static inline int sdsHdrSize(char type) {
    switch(type&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return sizeof(struct sdshdr5);
        case SDS_TYPE_8:
            return sizeof(struct sdshdr8);
        case SDS_TYPE_16:
            return sizeof(struct sdshdr16);
        case SDS_TYPE_32:
            return sizeof(struct sdshdr32);
        case SDS_TYPE_64:
            return sizeof(struct sdshdr64);
    }
    return 0;
}

static inline char sdsReqType(size_t string_size) {
    if (string_size < 32)
        return SDS_TYPE_5;
    if (string_size < 0xff)
        return SDS_TYPE_8;
    if (string_size < 0xffff)
        return SDS_TYPE_16;
    if (string_size < 0xffffffff)
        return SDS_TYPE_32;
    return SDS_TYPE_64;
}


sds sdsnewlen(const void *init, size_t initlen) {
    void *sh;
    sds s;
    char type = sdsReqType(initlen);
    /* Empty strings are usually created in order to append. Use type 8
     *      * since type 5 is not good at this. */
    if (type == SDS_TYPE_5 && initlen == 0) type = SDS_TYPE_8;
    int hdrlen = sdsHdrSize(type);
    unsigned char *fp; /* flags pointer. */

    sh = s_malloc(hdrlen+initlen+1);
    if (sh == NULL) return NULL;
    if (!init)
        memset(sh, 0, hdrlen+initlen+1);
    s = (char*)sh+hdrlen;

    fp = ((unsigned char*)s)-1;
    switch(type) {
        case SDS_TYPE_5: {
                     *fp = type | (initlen << SDS_TYPE_BITS);
                     break;
                 }
        case SDS_TYPE_8: {
                     SDS_HDR_VAR(8,s);
                     sh->len = initlen;
                     sh->alloc = initlen;
                     *fp = type;
                     break;
                 }
        case SDS_TYPE_16: {
                      SDS_HDR_VAR(16,s);
                      sh->len = initlen;
                      sh->alloc = initlen;
                      *fp = type;
                      break;
                  }
        case SDS_TYPE_32: {
                      SDS_HDR_VAR(32,s);
                      sh->len = initlen;
                      sh->alloc = initlen;
                      *fp = type;
                      break;
                  }
        case SDS_TYPE_64: {
                      SDS_HDR_VAR(64,s);
                      sh->len = initlen;
                      sh->alloc = initlen;
                      *fp = type;
                      break;
                  }
    }

    if (initlen && init)
        memcpy(s, init, initlen);
    s[initlen] = '\0';

    return s;
}

sds sdsnew(const char *init) {
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return sdsnewlen(init, initlen);
}

sds sdstrim(sds s, const char *cset) {
    char *start, *end, *sp, *ep;
    size_t len;

    sp = start = s;
    ep = end = s+sdslen(s)-1;
    /* 从头部和尾部逐个字符遍历往中间靠拢，如果字符在cest中，则继续前进 */
    while(sp <= end && strchr(cset, *sp)) sp++;
    while(ep > sp && strchr(cset, *ep)) ep--;
    len = (sp > ep) ? 0 : ((ep-sp)+1); // 全部被去除了，长度就是0
    if (s != sp) memmove(s, sp, len); // 拷贝内容
    s[len] = '\0';
    sdssetlen(s,len);
    return s;
}

char *str_trim(char* s, const char *cset) {
    char *start, *end, *sp, *ep;
    size_t len;

    sp = start = s;
    ep = end = s+strlen(s)-1;
    /* 从头部和尾部逐个字符遍历往中间靠拢，如果字符在cest中，则继续前进 */
    while(sp <= end && strchr(cset, *sp)) sp++;
    while(ep > sp && strchr(cset, *ep)) ep--;
    len = (sp > ep) ? 0 : ((ep-sp)+1); // 全部被去除了，长度就是0
    if (s != sp) memmove(s, sp, len); // 拷贝内容
    s[len] = '\0';

    return s;
}

int main()
{
    char *str = (char *)malloc(35 * sizeof(char *));
    memcpy(str, "AA...AA.a.aa.aHelloWorld     :::", 35);
    char *trim_str = str_trim(str, "Aa. :");
    printf("%s\n", trim_str);

    sds s;
    s = sdsnew("AA...AA.a.aa.aHelloWorld     :::");
    s = sdstrim(s,"Aa. :");
    printf("%s\n", s);

    return 0;
}