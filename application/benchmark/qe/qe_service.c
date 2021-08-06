/*
 * Copyright (C) 2021, Luster SCBU Development Team
 *
 * License: Apache-2.0
 *
 * Change Logs:
 * Date			Author		Note
 * 2021-07-20	Weizhou		the first version
 *
 */
 
#include "qe_core.h"
#include <stdarg.h>



/* global errno */
static volatile int qe_errno;

qe_err_t qe_get_errno(void)
{
	return qe_errno;
}

void qe_set_errno(qe_err_t e)
{
	qe_errno = e;
}

int *_qe_errno(void)
{
    return (int *)&qe_errno;
}

/**
 * This function will set the content of memory to specified value
 * 
 * @s     : the address of source memory
 * @c     : the value shall be set in content
 * @count : the copied length
 *
 * @return the address of source memory
 */
void *qe_memset(void *s, int c, qe_ubase_t count)
{
#ifdef QE_SERVICE_WITH_TINY
	char *xs = (char *)s;
	while (count--)
		*xs++ = c;
	return s;
#else
#define LBLOCKSIZE		(sizeof(long))
#define UNALIGNED(x)	((long)x & (LBLOCKSIZE-1))
#define TOO_SMALL(len)  ((len) < LBLOCKSIZE)

	unsigned int i;
	char *m = (char *)s;
	unsigned long buffer;
	unsigned long *aligned_addr;
	unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an unsigned variable */

	if (!TOO_SMALL(count) && !UNALIGNED(s)) {

		/* If we get this far, we know that count is large and s is word-aligned. */
		aligned_addr = (unsigned long *)s;

		/* Store d into each char sized location in buffer so that
         * we can set large blocks quickly.
         */
		if (LBLOCKSIZE == 4) {
			buffer = (d << 8) | d;
			buffer |= (buffer << 16);
		} else {
			buffer = 0;
			for (i = 0; i < LBLOCKSIZE; i ++)
				buffer = (buffer << 8) | d;
		}

		while (count >= LBLOCKSIZE * 4) {
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			count -= 4 * LBLOCKSIZE;
		}

		while (count >= LBLOCKSIZE) {
            *aligned_addr++ = buffer;
            count -= LBLOCKSIZE;
        }

		/* Pick up the remainder with a bytewise loop. */
		m = (char *)aligned_addr;
	}

	while (count--)
		*m++ = (char)d;
	return s;
#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL
#endif
}

/**
 * This function will copy memory content from source address to destination
 * address.
 *
 * @dst   : the address of destination memory
 * @src   : the address of source memory
 * @count : the copied length
 *
 * @return the address of destination memory
 */
void *qe_memcpy(void *dst, void *src, qe_ubase_t count)
{
#ifdef QE_SERVICE_WITH_TINY
	char *tmp = (char *)dst, *s = (char *)src;
	qe_ubase_t len;
	if (tmp <= s || tmp > (s + count)) {
		while (count--)
			*tmp ++ = *s ++;
	} else {
		for (len = count; len > 0; len --)
			tmp[len - 1] = s[len - 1];
	}
	return dst;
#else
#define UNALIGNED(X, Y) \
    (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))
#define BIGBLOCKSIZE    (sizeof (long) << 2)
#define LITTLEBLOCKSIZE (sizeof (long))
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)
	char *dst_ptr = (char *)dst;
    char *src_ptr = (char *)src;
    long *aligned_dst;
    long *aligned_src;
    int len = count;

	/* If the size is small, or either SRC or DST is unaligned,
	then punt into the byte copy loop.	This should be rare. */
	if (!TOO_SMALL(len) && !UNALIGNED(src_ptr, dst_ptr)) 
	{
		aligned_dst = (long *)dst_ptr;
		aligned_src = (long *)src_ptr;
		
		/* Copy 4X long words at a time if possible. */
		while (len >= BIGBLOCKSIZE) 
		{
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            len -= BIGBLOCKSIZE;			
		}

		/* Copy one long word at a time if possible. */
        while (len >= LITTLEBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            len -= LITTLEBLOCKSIZE;
        }

		/* Pick up any residual with a byte copier. */
		dst_ptr = (char *)aligned_dst;
		src_ptr = (char *)aligned_src;
	}

	while (len--)
		*dst_ptr++ = *src_ptr++;

	return dst;
#undef UNALIGNED
#undef BIGBLOCKSIZE
#undef LITTLEBLOCKSIZE
#undef TOO_SMALL
#endif
}

/**
 * This function will move memory content from source address to destination
 * address
 *
 * @dst : the address of destination memory
 * @src : the address of source memory
 * @n   : the copied length
 *
 * @return the address of destination memory
 */
void *qe_memmove(void *dst, void *src, qe_ubase_t n)
{
    char *tmp = (char *)dst, *s = (char *)src;

    if (s < tmp && tmp < s + n)
    {
        tmp += n;
        s += n;

        while (n--)
            *(--tmp) = *(--s);
    }
    else
    {
        while (n--)
            *tmp++ = *s++;
    }

    return dst;	
}

/**
 * This function will compare two areas of memory
 * 
 * @cs    : one area of memory
 * @ct    : another area of memory
 * @count : the size of the area
 *
 * @return result
 */
qe_int32_t qe_memcmp(const void *cs, const void *ct, qe_ubase_t count)
{
	int res = 0;
	const unsigned char *su1, *su2;

    for (su1 = (const unsigned char *)cs, su2 = (const unsigned char *)ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;

	return res;
}

/**
 * This function will return the first occurrence of a string.
 *
 * @s1 : the source string
 * @s2 : the find string
 *
 * @return the first occurrence of a s2 in s1, or RT_NULL if no found.
 */
char *qe_strstr(const char *s1, const char *s2)
{
	int l1, l2;

	l2 = qe_strlen(s2);
	if (!l2)
		return (char *)s1;
	l1 = qe_strlen(s1);
	while (l1 >= l2)
	{
		l1 --;
		if (!qe_memcmp(s1, s2, l2))
			return (char *)s1;
		s1 ++;
	}

	return QE_NULL;
}

/**
  * This function will copy string no more than n bytes.
  *
  * @dst :the string to copy
  * @src :the string to be copied
  * @n   :the maximum copied length
  *
  * @return the result
  */
char *qe_strncpy(char *dst, const char *src, qe_ubase_t n)
{
	if (n != 0)
	{
		char *d = dst;
		const char *s = src;

		do
		{
			if ((*d++ = *s++) == 0)
			{
				/* NUL pad the remaining n-1 bytes */
				while (--n != 0)
				*d++ = 0;
				break;
			}
		} while (--n != 0);
	}

	return (dst);
}

/**
 * This function will compare two strings with specified maximum length
 *
 * @cs    :the string to be compared
 * @ct    :the string to be compared
 * @count :the maximum compare length
 *
 * @return the result
 */
qe_int32_t qe_strncmp(const char *cs, const char *ct, qe_uint32_t count)
{
    register signed char __res = 0;

    while (count)
    {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
        count --;
    }

    return __res;
}

/**
 * This function will compare two strings without specified length
 *
 * @cs : the string to be compared
 * @ct : the string to be compared
 *
 * @return the result
 */
qe_int32_t qe_strcmp(const char *cs, const char *ct)
{
    while (*cs && *cs == *ct)
    {
        cs++;
        ct++;
    }

    return (*cs - *ct);
}

/**
 * The  strnlen()  function  returns the number of characters in the
 * string pointed to by s, excluding the terminating null byte ('\0'),
 * but at most maxlen.  In doing this, strnlen() looks only at the
 * first maxlen characters in the string pointed to by s and never
 * beyond s+maxlen.
 *
 * @s      : the string
 * @maxlen : the max size
 *
 * @return the length of string
 */
qe_size_t qe_strnlen(const char *s, qe_ubase_t maxlen)
{
    const char *sc;

    for (sc = s; *sc != '\0' && (qe_ubase_t)(sc - s) < maxlen; ++sc) /* nothing */
        ;

    return sc - s;
}

/**
 * This function will return the length of a string, which terminate will
 * null character.
 *
 * @param s the string
 *
 * @return the length of string
 */
qe_size_t qe_strlen(const char *s)
{
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc) /* nothing */
        ;

    return sc - s;
}

/**
 * This function will duplicate a string.
 *
 * @s : the string to be duplicated
 *
 * @return the duplicated string pointer
 */
char *qe_strdup(const char *s)
{
    qe_size_t len = qe_strlen(s) + 1;
    char *tmp = (char *)qe_malloc(len);

    if (!tmp)
        return QE_NULL;

    qe_memcpy(tmp, (void *)s, len);

    return tmp;
}


/**
 * This function will show the version of QESF
 */
#if 0
void qe_show_version(void)
{
    qe_printf("QESF@%d.%d.%d build %s\n",
		QE_VERSION, QE_SUBVERSION, QE_REVISION, __DATE__);
    qe_printf(" Copyright (C) 2021, Luster SCBU Team\n");
}
#endif

#define _ISDIGIT(c)  ((unsigned)((c) - '0') < 10)

#if (QE_PRINTF_LONGLONG == 1)
inline int divide(long long *n, int base)
{
    int res;

    /* optimized for processor which does not support divide instructions. */
    if (base == 10)
    {
        res = (int)(((unsigned long long)*n) % 10U);
        *n = (long long)(((unsigned long long)*n) / 10U);
    }
    else
    {
        res = (int)(((unsigned long long)*n) % 16U);
        *n = (long long)(((unsigned long long)*n) / 16U);
    }

    return res;
}
#else
inline int divide(long *n, int base)
{
    int res;

    /* optimized for processor which does not support divide instructions. */
    if (base == 10)
    {
        res = (int)(((unsigned long)*n) % 10U);
        *n = (long)(((unsigned long)*n) / 10U);
    }
    else
    {
        res = (int)(((unsigned long)*n) % 16U);
        *n = (long)(((unsigned long)*n) / 16U);
    }

    return res;
}
#endif

inline int skip_atoi(const char **s)
{
    register int i = 0;
    while (_ISDIGIT(**s))
        i = i * 10 + *((*s)++) - '0';

    return i;
}

#define ZEROPAD     (1 << 0)    /* pad with zero */
#define SIGN        (1 << 1)    /* unsigned/signed long */
#define PLUS        (1 << 2)    /* show plus */
#define SPACE       (1 << 3)    /* space if plus */
#define LEFT        (1 << 4)    /* left justified */
#define SPECIAL     (1 << 5)    /* 0x */
#define LARGE       (1 << 6)    /* use 'ABCDEF' instead of 'abcdef' */

#if (QE_PRINTF_PRECISION == 1)
static char *print_number(char *buf,
                          char *end,
#if (QE_PRINTF_LONGLONG == 1)
                          long long  num,
#else
                          long  num,
#endif
                          int   base,
                          int   s,
                          int   precision,
                          int   type)
#else
static char *print_number(char *buf,
                          char *end,
#if (QE_PRINTF_LONGLONG == 1)
                          long long  num,
#else
                          long  num,
#endif
                          int   base,
                          int   s,
                          int   type)
#endif
{
    char c, sign;
#if (QE_PRINTF_LONGLONG == 1)
    char tmp[32];
#else
    char tmp[16];
#endif
    int precision_bak = precision;
    const char *digits;
    static const char small_digits[] = "0123456789abcdef";
    static const char large_digits[] = "0123456789ABCDEF";
    register int i;
    register int size;

    size = s;

    digits = (type & LARGE) ? large_digits : small_digits;
    if (type & LEFT)
        type &= ~ZEROPAD;

    c = (type & ZEROPAD) ? '0' : ' ';

    /* get sign */
    sign = 0;
    if (type & SIGN)
    {
        if (num < 0)
        {
            sign = '-';
            num = -num;
        }
        else if (type & PLUS)
            sign = '+';
        else if (type & SPACE)
            sign = ' ';
    }

#ifdef QE_PRINTF_SPECIAL
    if (type & SPECIAL)
    {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }
#endif

    i = 0;
    if (num == 0)
        tmp[i++] = '0';
    else
    {
        while (num != 0)
            tmp[i++] = digits[divide(&num, base)];
    }

#if (QE_PRINTF_PRECISION == 1)
    if (i > precision)
        precision = i;
    size -= precision;
#else
    size -= i;
#endif

    if (!(type & (ZEROPAD | LEFT)))
    {
        if ((sign) && (size > 0))
            size--;

        while (size-- > 0)
        {
            if (buf < end)
                *buf = ' ';
            ++ buf;
        }
    }

    if (sign)
    {
        if (buf < end)
        {
            *buf = sign;
        }
        -- size;
        ++ buf;
    }

#ifdef QE_PRINTF_SPECIAL
    if (type & SPECIAL)
    {
        if (base == 8)
        {
            if (buf < end)
                *buf = '0';
            ++ buf;
        }
        else if (base == 16)
        {
            if (buf < end)
                *buf = '0';
            ++ buf;
            if (buf < end)
            {
                *buf = type & LARGE ? 'X' : 'x';
            }
            ++ buf;
        }
    }
#endif

    /* no align to the left */
    if (!(type & LEFT))
    {
        while (size-- > 0)
        {
            if (buf < end)
                *buf = c;
            ++ buf;
        }
    }

#if (QE_PRINTF_PRECISION == 1)
    while (i < precision--)
    {
        if (buf < end)
            *buf = '0';
        ++ buf;
    }
#endif

    /* put number in the temporary buffer */
    while (i-- > 0 && (precision_bak != 0))
    {
        if (buf < end)
            *buf = tmp[i];
        ++ buf;
    }

    while (size-- > 0)
    {
        if (buf < end)
            *buf = ' ';
        ++ buf;
    }

    return buf;
}

qe_int32_t qe_vsnprintf(char       *buf,
                        	 qe_size_t   size,
                        	 const char *fmt,
                        	 va_list     args)
{
#if (QE_PRINTF_LONGLONG == 1)
    unsigned long long num;
#else
    qe_uint32_t num;
#endif
    int i, len;
    char *str, *end, c;
    const char *s;

    qe_uint8_t base;            /* the base of number */
    qe_uint8_t flags;           /* flags to print number */
    qe_uint8_t qualifier;       /* 'h', 'l', or 'L' for integer fields */
    qe_int32_t field_width;     /* width of output field */

#if (QE_PRINTF_PRECISION == 1)
    int precision;      /* min. # of digits for integers and max for a string */
#endif

    str = buf;
    end = buf + size;

    /* Make sure end is always >= buf */
    if (end < buf)
    {
        end  = ((char *) - 1);
        size = end - buf;
    }

    for (; *fmt ; ++fmt)
    {
        if (*fmt != '%')
        {
            if (str < end)
                *str = *fmt;
            ++ str;
            continue;
        }

        /* process flags */
        flags = 0;

        while (1)
        {
            /* skips the first '%' also */
            ++ fmt;
            if (*fmt == '-') flags |= LEFT;
            else if (*fmt == '+') flags |= PLUS;
            else if (*fmt == ' ') flags |= SPACE;
            else if (*fmt == '#') flags |= SPECIAL;
            else if (*fmt == '0') flags |= ZEROPAD;
            else break;
        }

        /* get field width */
        field_width = -1;
        if (_ISDIGIT(*fmt)) field_width = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
            ++ fmt;
            /* it's the next argument */
            field_width = va_arg(args, int);
            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

#if (QE_PRINTF_PRECISION == 1)
        /* get the precision */
        precision = -1;
        if (*fmt == '.')
        {
            ++ fmt;
            if (_ISDIGIT(*fmt)) precision = skip_atoi(&fmt);
            else if (*fmt == '*')
            {
                ++ fmt;
                /* it's the next argument */
                precision = va_arg(args, int);
            }
            if (precision < 0) precision = 0;
        }
#endif
        /* get the conversion qualifier */
        qualifier = 0;
#if (QE_PRINTF_LONGLONG == 1)
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
#else
        if (*fmt == 'h' || *fmt == 'l')
#endif
        {
            qualifier = *fmt;
            ++ fmt;
#if (QE_PRINTF_LONGLONG == 1)
            if (qualifier == 'l' && *fmt == 'l')
            {
                qualifier = 'L';
                ++ fmt;
            }
#endif
        }

        /* the default base */
        base = 10;

        switch (*fmt)
        {
        case 'c':
            if (!(flags & LEFT))
            {
                while (--field_width > 0)
                {
                    if (str < end) *str = ' ';
                    ++ str;
                }
            }

            /* get character */
            c = (qe_uint8_t)va_arg(args, int);
            if (str < end) *str = c;
            ++ str;

            /* put width */
            while (--field_width > 0)
            {
                if (str < end) *str = ' ';
                ++ str;
            }
            continue;

        case 's':
            s = va_arg(args, char *);
            if (!s) s = "(NULL)";

            len = qe_strlen(s);
#if (QE_PRINTF_PRECISION == 1)
            if (precision > 0 && len > precision) len = precision;
#endif

            if (!(flags & LEFT))
            {
                while (len < field_width--)
                {
                    if (str < end) *str = ' ';
                    ++ str;
                }
            }

            for (i = 0; i < len; ++i)
            {
                if (str < end) *str = *s;
                ++ str;
                ++ s;
            }

            while (len < field_width--)
            {
                if (str < end) *str = ' ';
                ++ str;
            }
            continue;

        case 'p':
            if (field_width == -1)
            {
                field_width = sizeof(void *) << 1;
                flags |= ZEROPAD;
            }
#if (QE_PRINTF_PRECISION == 1)
            str = print_number(str, end,
                               (long)va_arg(args, void *),
                               16, field_width, precision, flags);
#else
            str = print_number(str, end,
                               (long)va_arg(args, void *),
                               16, field_width, flags);
#endif
            continue;

        case '%':
            if (str < end) *str = '%';
            ++ str;
            continue;

        /* integer number formats - set up the flags and "break" */
        case 'o':
            base = 8;
            break;

        case 'X':
            flags |= LARGE;
        case 'x':
            base = 16;
            break;

        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            break;

        default:
            if (str < end) *str = '%';
            ++ str;

            if (*fmt)
            {
                if (str < end) *str = *fmt;
                ++ str;
            }
            else
            {
                -- fmt;
            }
            continue;
        }

#if (QE_PRINTF_LONGLONG == 1)
        if (qualifier == 'L') num = va_arg(args, long long);
        else if (qualifier == 'l')
#else
        if (qualifier == 'l')
#endif
        {
            num = va_arg(args, qe_uint32_t);
            if (flags & SIGN) num = (qe_int32_t)num;
        }
        else if (qualifier == 'h')
        {
            num = (qe_uint16_t)va_arg(args, qe_int32_t);
            if (flags & SIGN) num = (qe_int16_t)num;
        }
        else
        {
            num = va_arg(args, qe_uint32_t);
            if (flags & SIGN) num = (qe_int32_t)num;
        }
#if (QE_PRINTF_PRECISION == 1)
        str = print_number(str, end, num, base, field_width, precision, flags);
#else
        str = print_number(str, end, num, base, field_width, flags);
#endif
    }

    if (size > 0)
    {
        if (str < end) *str = '\0';
        else
        {
            end[-1] = '\0';
        }
    }

    /* the trailing null byte doesn't count towards the total
    * ++str;
    */
    return str - buf;
}

/**
 * This function will fill a formatted string to buffer
 *
 * @buf  : the buffer to save formatted string
 * @size : the size of buffer
 * @fmt  : the format
 */
qe_int32_t qe_snprintf(char *buf, qe_size_t size, const char *fmt, ...)
{
    qe_int32_t n;
    va_list args;

    va_start(args, fmt);
    n = qe_vsnprintf(buf, size, fmt, args);
    va_end(args);

    return n;
}

/**
 * This function will fill a formatted string to buffer
 *
 * @buf     : the buffer to save formatted string
 * @arg_ptr : the arg_ptr
 * @format  : the format
 */
qe_int32_t qe_vsprintf(char *buf, const char *format, va_list arg_ptr)
{
    return qe_vsnprintf(buf, (qe_size_t) - 1, format, arg_ptr);
}

/**
 * This function will fill a formatted string to buffer
 *
 * @param buf the buffer to save formatted string
 * @param format the format
 */
qe_int32_t rt_sprintf(char *buf, const char *format, ...)
{
    qe_int32_t n;
    va_list arg_ptr;

    va_start(arg_ptr, format);
    n = qe_vsprintf(buf, format, arg_ptr);
    va_end(arg_ptr);

    return n;
}

inline qe_strb_t qe_strb_make(char *s, int max)
{
	qe_strb_t bpr;

	bpr.p = s;
	bpr.head = s;
	bpr.max = max;
	bpr.len = qe_strlen(s);
	return bpr;
}

inline int qe_strb_build(qe_strb_t *bpr, const char *fmt, ...)
{
	int len;
	va_list ap;
	
	va_start(ap, fmt);
	len = qe_vsnprintf(bpr->p, bpr->max, fmt, ap);
	va_end(ap);

	bpr->max -= len;
	bpr->len += len;
	bpr->p += len;
	
	return len;
}

void (*qe_assert_hook)(const char *ex, const char *func, qe_size_t line);

/**
 * This function will set a hook to assert.
 *
 * @hook: the hook function
 */
void qe_assert_set_hook(void (*hook)(const char *, const char *, qe_size_t))
{
	qe_assert_hook = hook;
}

void qe_assert_handler(const char *ex_string, const char *func, qe_size_t line)
{
	volatile char dummy = 0;
	
	if (qe_assert_hook == QE_NULL) {
		//sys_bug("Assert(%s) %s:%d", ex_string, func, line);
		while (dummy == 0);
	} else {
		qe_assert_hook(ex_string, func, line);
	}
}

