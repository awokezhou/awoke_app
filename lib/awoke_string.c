#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

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

