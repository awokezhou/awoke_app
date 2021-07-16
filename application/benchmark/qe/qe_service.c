
#include "qe_service.h"


char *qe_strncpy(char *dst, const char *src, uint32_t n)
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

int32_t qe_strncmp(const char *cs, const char *ct, uint32_t count)
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

