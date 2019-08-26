
#include "awoke_string.h"
#include "awoke_memory.h"
#include "awoke_macros.h"
#include "awoke_type.h"


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

int awoke_string_from_hex(const uint8_t *hex, char *string, uint16_t hex_len)
{
	int i;
	int str_len = 0;

	if (!hex || !string || hex_len<=0)
		return -1;

	for (i=0; i<hex_len; i++) {
		str_len += sprintf(string+str_len, "%X", (uint8_t)(hex[i] & 0xF0) >> 4); 
		str_len += sprintf(string+str_len, "%X", (uint8_t)(hex[i] & 0x0F));       
	}

	string[str_len] = '\0';
	return str_len;
}

static int string_htoi(char *src)
{ 
	int n = 0; 
  	char *str = src;
  
  	for (; (*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >='A' && *str <= 'F');str++)  
  	{	
     	char a = tolower((uint8_t)*str);
	 	if (a > '9')  	
		 	n = 16*n + (10+a-'a');  
	 	else  	
		 	n = 16*n + (a-'0');  
  	}	
	
  	return n;	
}

int awoke_string_to_hex(char *string, uint8_t *hex, uint16_t hex_len)
{
	int i;
    int len;
    char *str = string;
	
    if (!hex || !string || hex_len<=0)
        return -1;

	if(0 != (hex_len%2))
        return -1;

    for (i=0; ((i < (hex_len/2))&&( 0 != *str)); i++)
    {
        char hex_c[3] = {0};

		strncpy(hex_c,str,2);
		str = str + 2;

		hex[i] = (uint8_t)string_htoi(hex_c);
        len++;
    }

    return len;
}

