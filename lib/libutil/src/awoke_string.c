
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

static int ctoi(char c)
{
    int i;

    i = (int)c - '0';
    if (i < 0 || i > 9)
    {
        i = 0;
    }
    return i;
}

int awoke_string_to_hex(char *string, uint8_t *hex, uint16_t hex_len)
{
	int i;
    int len;
    char *str = string;
	
    if (!hex || !string || hex_len<=0) {
        return -1;
    }

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

int awoke_string_bcd2str(char *out, const uint8_t *in, uint16_t b_len)
{
	int i;
    int out_len = 0;

    if (out == NULL || in == NULL || b_len == 0)
    {
        return -1;
    }

    for (i = 0; i < b_len/2; i++)
    {
        out_len += sprintf(out + out_len, "%u", (uint8_t)(in[i] & 0x0F));        // lower nibble
        out_len += sprintf(out + out_len, "%u", (uint8_t)(in[i] & 0xF0) >> 4);   // upper nibbl
    }
    // If it is an odd number add one digit more
    if ((b_len & 0x1) != 0)
    {
        out_len += sprintf(out + out_len, "%u", (uint8_t)(in[b_len/2] & 0x0F));  // lower nibble
    }
    out[out_len] = '\0'; // It is a string so add the null terminated character
    return out_len;
}

int awoke_string_str2bcd(uint8_t *bin_data, const char *str_data, uint16_t bin_len)
{
    int out_len;

    uint8_t digit1;
    uint8_t digit2;

    if (bin_data == NULL || str_data == NULL || bin_len == 0)
    {
        return -1;
    }

    for ( out_len = 0; out_len +1 < bin_len; out_len += 2)       //lint !e440  False positive complaint about bit_len not being modified.
    {
        if ((str_data[out_len] < '0') || (str_data[out_len] > '9') || (str_data[out_len + 1] < '0') || (str_data[out_len + 1] > '9'))
        {
            return -1;
        }

        digit1 = (uint8_t)ctoi(str_data[out_len]);
        digit2 = (uint8_t)ctoi(str_data[out_len + 1]);
        bin_data[out_len/2] = digit1 | ((digit2 << 4) & 0xFF);
    }
    // If it is an odd number add the last digit and a 0
    if ((bin_len & 0x1) != 0)
    {
        if ((str_data[bin_len - 1] < '0') || (str_data[bin_len - 1] > '9'))
        {
            return -1;
        }

        bin_data[bin_len/2] = (uint8_t)ctoi(str_data[bin_len - 1]); // just digit 1, digit2 = 0
    }
    return bin_len;
}

