
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "awoke_type.h"

typedef enum {
    HEADER_ACCEPT             = 1,
	HEADER_HOST                  ,
    HEADER_ACCEPT_CHARSET        ,
    HEADER_ACCEPT_ENCODING       ,
    HEADER_ACCEPT_LANGUAGE       ,
    HEADER_CONNECTION            ,
    HEADER_CONTENT_LENGTH        ,
    HEADER_CONTENT_TYPE          ,
    HEADER_SIZEOF                ,

    /* used by the core for custom headers */
    HEADER_OTHER
} request_headers;

typedef struct _http_header {
    /* The header type/name, e.g: HEADER_CONTENT_LENGTH */
    int type;

    /* Reference the header Key name, e.g: 'Content-Length' */
    mem_ptr_t key;

    /* Reference the header Value, e/g: '123456' */
    mem_ptr_t val;
} http_header;

typedef struct _http_network_io {
	int (*create_conn)(struct _http_connection *);
	int (*connect_udp)(struct _http_connection *);
	
} http_network_io;

#define HTTP_PROTOCOL_11_STR "HTTP/1.1"		

#define METHOD_GET_STR       "GET"
#define METHOD_POST_STR      "POST"
#define METHOD_HEAD_STR      "HEAD"
#define METHOD_PUT_STR       "PUT"
#define METHOD_DELETE_STR    "DELETE"
#define METHOD_OPTIONS_STR   "OPTIONS"

/* Headers */
#define RH_ACCEPT "Accept:"
#define RH_ACCEPT_CHARSET "Accept-Charset:"
#define RH_ACCEPT_ENCODING "Accept-Encoding:"
#define RH_ACCEPT_LANGUAGE "Accept-Language:"
#define RH_CONNECTION "Connection:"
#define RH_COOKIE "Cookie:"
#define RH_CONTENT_LENGTH "Content-Length:"
#define RH_CONTENT_RANGE "Content-Range:"
#define RH_CONTENT_TYPE	"Content-Type:"
#define RH_IF_MODIFIED_SINCE "If-Modified-Since:"
#define RH_HOST	"Host:"
#define RH_LAST_MODIFIED "Last-Modified:"
#define RH_LAST_MODIFIED_SINCE "Last-Modified-Since:"
#define RH_REFERER "Referer:"
#define RH_RANGE "Range:"
#define RH_USER_AGENT "User-Agent:"

typedef struct _http_request {

	/* -- start line -- */
	mem_ptr_t method;
	mem_ptr_t path;
	mem_ptr_t protocol;

	/* -- request headers -- */
	http_header headers[HEADER_SIZEOF]; 

	/* -- POST data -- */
	mem_ptr_t body;
	
#define HTTP_BUFF_MAX	8092	
	char *pos;
	int original_len;
	char original_buf[HTTP_BUFF_MAX];
} http_request; 

typedef struct _http_connection {
	int sock;
	uint32_t flags;
	struct sockaddr_in addr;
} http_conn;

#endif /* __HTTP_CLIENT_H__ */