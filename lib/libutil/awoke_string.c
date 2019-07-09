
#include "awoke_string.h"
#include "awoke_memory.h"
#include "awoke_macros.h"



char *awoke_string_dup(const char *s)
{
    size_t len;
    char *p;

    if (!s)
        return NULL;

    len = strlen(s);
    p = mem_alloc(len + 1);
    memcpy(p, s, len);
    p[len] = '\0';

    return p;
}

char *awoke_string_build(char **buffer, unsigned long *len,
                      const char *format, ...)
{
    va_list ap;
    int length;
    char *ptr;
    const size_t _mem_alloc = 64;
    size_t alloc = 0;

    /* *buffer *must* be an empty/NULL buffer */
    //awoke_unlikely(*buffer);
    *buffer = (char *) mem_alloc(_mem_alloc);

    if (!*buffer) {
        return NULL;
    }
    alloc = _mem_alloc;

    va_start(ap, format);
    length = vsnprintf(*buffer, alloc, format, ap);
    va_end(ap);

    if (length < 0) {
        return NULL;
    }

    if ((unsigned int) length >= alloc) {
        ptr = mem_realloc(*buffer, length + 1);
        if (!ptr) {
            return NULL;
        }
        *buffer = ptr;
        alloc = length + 1;

        va_start(ap, format);
        length = vsnprintf(*buffer, alloc, format, ap);
        va_end(ap);
    }

    ptr = *buffer;
    ptr[length] = '\0';
    *len = length;

    return *buffer;
}

/* Lookup char into string, return position */
int awoke_string_char_search(const char *string, int c, int len)
{
    char *p;

    if (len < 0) {
        len = strlen(string);
    }

    p = memchr(string, c, len);
    if (p) {
        return (p - string);
    }

    return -1;
}

