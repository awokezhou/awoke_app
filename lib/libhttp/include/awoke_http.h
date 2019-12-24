
#ifndef __AWOKE_HTTP_H__
#define	__AWOKE_HTTP_H__

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "awoke_tcp.h"
#include "awoke_list.h"
#include "awoke_memory.h"
#include "awoke_string.h"




#define STR_CRLD	"\r\n"



/* -- protocol define --{ */
#define HTTP_PROTOCOL_10_STR 	"HTTP/1.0"	
#define HTTP_PROTOCOL_11_STR	"HTTP/1.1"		
#define HTTP_PROTOCOL_20_STR 	"HTTP/2.0"	
/* }-- protocol define -- */


/* -- statecode define --{ */
#define HTTP_CODE_SUCCESS	200
/* }-- statecode define -- */


/* -- method define --{ */
#define HTTP_METHOD_GET_STR		"GET"
#define HTTP_METHOD_POST_STR    "POST"
#define HTTP_METHOD_HEAD_STR    "HEAD"
#define HTTP_METHOD_PUT_STR     "PUT"
#define HTTP_METHOD_DELETE_STR  "DELETE"
#define HTTP_METHOD_OPTIONS_STR "OPTIONS"

typedef enum {
	HTTP_METHOD_GET = 1,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_OPTIONS,
} http_method;
/* }-- method define -- */


/* -- headers define --{ */
#define HTTP_HEADER_ACCEPT_STR 				"Accept:"
#define HTTP_HEADER_ACCEPT_CHARSET_STR 		"Accept-Charset:"
#define HTTP_HEADER_ACCEPT_ENCODING_STR 	"Accept-Encoding:"
#define HTTP_HEADER_ACCEPT_LANGUAGE_STR 	"Accept-Language:"
#define HTTP_HEADER_CONNECTION_STR 			"Connection:"
#define HTTP_HEADER_COOKIE_STR 				"Cookie:"
#define HTTP_HEADER_CONTENT_LENGTH_STR 		"Content-Length:"
#define HTTP_HEADER_CONTENT_RANGE_STR 		"Content-Range:"
#define HTTP_HEADER_CONTENT_TYPE_STR		"Content-Type:"
#define HTTP_HEADER_IF_MODIFIED_SINCE_STR 	"If-Modified-Since:"
#define HTTP_HEADER_HOST_STR				"Host:"
#define HTTP_HEADER_LAST_MODIFIED_STR		"Last-Modified:"
#define HTTP_HEADER_LAST_MODIFIED_SINCE_STR "Last-Modified-Since:"
#define HTTP_HEADER_REFERER_STR 			"Referer:"
#define HTTP_HEADER_RANGE_STR 				"Range:"
#define HTTP_HEADER_USER_AGENT_STR 			"User-Agent:"
#define HTTP_HEADER_KEEP_ALIVE_STR			"Keep-Alive:"
#define HTTP_HEADER_AUTHORIZATION_STR		"Authorization:"

typedef enum {
	HTTP_HEADER_UNKNOWN = 0,
    HTTP_HEADER_ACCEPT,
	HTTP_HEADER_HOST,
    HTTP_HEADER_ACCEPT_CHARSET,
    HTTP_HEADER_ACCEPT_ENCODING,
    HTTP_HEADER_ACCEPT_LANGUAGE,
    HTTP_HEADER_CONNECTION,
    HTTP_HEADER_CONTENT_LENGTH,
    HTTP_HEADER_CONTENT_TYPE,
    HTTP_HEADER_USER_AGENT,
    HTTP_HEADER_KEEP_ALIVE,
    HTTP_HEADER_AUTHORIZATION,
    HTTP_HEADER_SIZEOF,

    /* used by the core for custom headers */
    HTTP_HEADER_OTHER
} request_headers;

typedef struct _http_header_entry {
	char* key;
	int type;
} http_header_entry;

typedef struct _http_header {
    int type;		/* header type/name, e.g: HEADER_CONTENT_LENGTH */
    mem_ptr_t key;	/* reference the header Key name, e.g: 'Content-Length' */
    mem_ptr_t val;	/* reference the header Value, e.g: '123456' */
} http_header;

#define http_header_entry_make(entry)	{entry##_STR, entry}
/* }-- headers define -- */


/* -- connect define --{ */
typedef struct _http_connect {
	const char *uri;
	
	char hostname[32];
	
	mem_ptr_t host;
	mem_ptr_t user;
	mem_ptr_t path;
	mem_ptr_t query;
	mem_ptr_t scheme;
	mem_ptr_t fragment;
	
#define HTTP_CONN_F_IPV4		0x0001
#define HTTP_CONN_F_HOSTNAME	0x0002
#define HTTP_CONN_F_IMPORT		0x0004
#define HTTP_CONN_F_EXPORT		0x0008
	uint16_t flag;

	awoke_tcp_connect _conn;
} http_connect;
/* }-- connect define -- */


/* -- request define --{ */
typedef struct _http_request {

	char *uri; 
	
	mem_ptr_t host;
	mem_ptr_t path;
	mem_ptr_t body;
	mem_ptr_t method;
	mem_ptr_t protocol;

	int state_code;
	
	http_connect conn;

	http_header *extend_headers;
	http_header headers[HTTP_HEADER_SIZEOF]; 

#define HTTP_REQPKT_SIZE	8092	
	build_ptr bp;
	int original_len;
	char original_buf[HTTP_REQPKT_SIZE];

	awoke_list _head;
}http_request_struct;
/* }-- request define -- */



/* public interface define --{ */
void http_header_init(struct _http_header *headers);
void http_header_set(struct _http_header *headers, int type, char *val);
void http_connect_release(struct _http_connect *c);
err_type http_do_connect(struct _http_connect *c);
err_type http_request(const char *uri, const char *method, const char *protocol, 
	const char *body, struct _http_header *headers, struct _http_connect *alive, char *rsp);
/* }-- public interface define */



#endif /* __AWOKE_HTTP_H__ */
