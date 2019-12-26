
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
#include "awoke_package.h"



#define STR_CRLD	"\r\n"


/* -- protocol define --{ */
#define HTTP_PROTOCOL_10_STR 	"HTTP/1.0"	
#define HTTP_PROTOCOL_11_STR	"HTTP/1.1"		
#define HTTP_PROTOCOL_20_STR 	"HTTP/2.0"	

typedef enum {
	HTTP_PROTOCOL_UNKNOWN = 0,
	HTTP_PROTOCOL_10,
	HTTP_PROTOCOL_11,
	HTTP_PROTOCOL_20,
} http_protocol;
/* }-- protocol define -- */


/* -- status code define --{ */
/* succesful */
#define HTTP_CODE_SUCCESS			200
#define HTTP_CODE_CREATED			201
#define HTTP_CODE_ACCEPTED			202
#define HTTP_CODE_NON_AUTH_INFO		203
#define HTTP_CODE_NOCONTENT			204
#define HTTP_CODE_RESET				205
#define HTTP_CODE_PARTIAL			206

/* redirections */
#define HTTP_CODE_REDIR_MULTIPLE	300
#define HTTP_CODE_REDIR_MOVED		301
#define HTTP_CODE_REDIR_MOVED_T		302
#define	HTTP_CODE_REDIR_SEE_OTHER	303
#define HTTP_CODE_NOT_MODIFIED		304
#define HTTP_CODE_REDIR_USE_PROXY	305

/* client errors */
#define HTTP_CODE_CLIENT_BAD_REQUEST				400
#define HTTP_CODE_CLIENT_UNAUTH						401
#define HTTP_CODE_CLIENT_PAYMENT_REQ   				402     
#define HTTP_CODE_CLIENT_FORBIDDEN					403
#define HTTP_CODE_CLIENT_NOT_FOUND					404
#define HTTP_CODE_CLIENT_METHOD_NOT_ALLOWED			405
#define HTTP_CODE_CLIENT_NOT_ACCEPTABLE				406
#define HTTP_CODE_CLIENT_PROXY_AUTH					407
#define HTTP_CODE_CLIENT_REQUEST_TIMEOUT			408
#define HTTP_CODE_CLIENT_CONFLICT					409
#define HTTP_CODE_CLIENT_GONE						410
#define HTTP_CODE_CLIENT_LENGTH_REQUIRED			411
#define HTTP_CODE_CLIENT_PRECOND_FAILED				412
#define HTTP_CODE_CLIENT_REQUEST_ENTITY_TOO_LARGE	413
#define HTTP_CODE_CLIENT_REQUEST_URI_TOO_LONG		414
#define HTTP_CODE_CLIENT_UNSUPPORTED_MEDIA			415
#define HTTP_CODE_CLIENT_REQUESTED_RANGE_NOT_SATISF 416

/* server errors */
#define HTTP_CODE_SERVER_INTERNAL_ERROR			500
#define HTTP_CODE_SERVER_NOT_IMPLEMENTED		501
#define HTTP_CODE_SERVER_BAD_GATEWAY			502
#define HTTP_CODE_SERVER_SERVICE_UNAV			503
#define HTTP_CODE_SERVER_GATEWAY_TIMEOUT		504
#define HTTP_CODE_SERVER_HTTP_VERSION_UNSUP		505

/* }-- status code define -- */


/* -- method define --{ */
#define HTTP_METHOD_GET_STR			"GET"
#define HTTP_METHOD_POST_STR    	"POST"
#define HTTP_METHOD_HEAD_STR    	"HEAD"
#define HTTP_METHOD_PUT_STR     	"PUT"
#define HTTP_METHOD_DELETE_STR  	"DELETE"
#define HTTP_METHOD_OPTIONS_STR 	"OPTIONS"

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
#define HTTP_HEADER_UNKNOWN_STR 			NULL
#define HTTP_HEADER_ACCEPT_STR 				"Accept"
#define HTTP_HEADER_ACCEPT_CHARSET_STR 		"Accept-Charset"
#define HTTP_HEADER_ACCEPT_ENCODING_STR 	"Accept-Encoding"
#define HTTP_HEADER_ACCEPT_LANGUAGE_STR 	"Accept-Language"
#define HTTP_HEADER_CONNECTION_STR 			"Connection"
#define HTTP_HEADER_COOKIE_STR 				"Cookie"
#define HTTP_HEADER_CONTENT_LENGTH_STR 		"Content-Length"
#define HTTP_HEADER_CONTENT_RANGE_STR 		"Content-Range"
#define HTTP_HEADER_CONTENT_TYPE_STR		"Content-Type"
#define HTTP_HEADER_IF_MODIFIED_SINCE_STR 	"If-Modified-Since"
#define HTTP_HEADER_HOST_STR				"Host"
#define HTTP_HEADER_LAST_MODIFIED_STR		"Last-Modified"
#define HTTP_HEADER_LAST_MODIFIED_SINCE_STR "Last-Modified-Since"
#define HTTP_HEADER_REFERER_STR 			"Referer"
#define HTTP_HEADER_RANGE_STR 				"Range"
#define HTTP_HEADER_USER_AGENT_STR 			"User-Agent"
#define HTTP_HEADER_KEEP_ALIVE_STR			"Keep-Alive"
#define HTTP_HEADER_AUTHORIZATION_STR		"Authorization"

typedef enum {
	HTTP_HEADER_UNKNOWN = 0,
	HTTP_HEADER_HOST,
	HTTP_HEADER_USER_AGENT,
	HTTP_HEADER_ACCEPT,
    HTTP_HEADER_ACCEPT_CHARSET,
    HTTP_HEADER_ACCEPT_ENCODING,
    HTTP_HEADER_ACCEPT_LANGUAGE,
    HTTP_HEADER_CONNECTION,
    HTTP_HEADER_CONTENT_LENGTH,
    HTTP_HEADER_CONTENT_TYPE,
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
#define HTTP_CONN_F_KEEP_ALIVE	0x0010
	uint16_t flag;

	awoke_tcp_connect _conn;
} http_connect;
/* }-- connect define -- */


/* -- http buffer chunk -- {*/
#define HTTP_BUFFER_CHUNK		8092		/* HTTP buffer chunks 4KB */
#define HTTP_BUFFER_LIMIT		8*HTTP_BUFFER_CHUNK

typedef struct _http_buffchunk {
	char *buff;
	char _fixed[HTTP_BUFFER_CHUNK];
	int size;
	int length;
} http_buffchunk;

#define http_buffchunk_init(chunk)	do {\
		chunk.size = HTTP_BUFFER_CHUNK;\
		chunk.length = 0;\
		chunk.buff = chunk._fixed;\
		memset(chunk._fixed, 0x0, HTTP_BUFFER_CHUNK);\
	} while(0)
/* }-- http buffer chunk */


/* -- request define --{ */
typedef struct _http_request {

	const char *uri; 
	
	mem_ptr_t host;
	mem_ptr_t path;
	mem_ptr_t body;
	mem_ptr_t method;
	mem_ptr_t protocol;
	
	http_connect conn;

	http_header *extend_headers;
	http_header headers[HTTP_HEADER_SIZEOF]; 
	
	build_ptr bp;
	http_buffchunk buffchunk;
	
	awoke_list _head;
}http_request_struct;
/* }-- request define -- */


/* response define --{ */
typedef struct _http_response {

	int status;
	int protocol;
	int connection;
	long content_length;

	mem_ptr_t firstline;
	mem_ptr_t protocol_p;
	mem_ptr_t statuscode_p;
	mem_ptr_t body;

	http_header headers[HTTP_HEADER_SIZEOF];
	
	http_buffchunk buffchunk;
	
} http_response;
/* }-- response define */


/* public interface define --{ */
void http_header_init(struct _http_header *headers);
void http_header_set(struct _http_header *headers, int type, char *val);
void http_connect_release(struct _http_connect *c);
err_type http_do_connect(struct _http_connect *c);
bool http_connect_keep_alive(struct _http_connect *c);
void http_connect_set_keep_alive(struct _http_connect *c);
err_type http_request_send(struct _http_request *req);
err_type http_response_recv(struct _http_request *req, struct _http_response *rsp);
err_type http_request(const char *uri, const char *method, const char *protocol, 
	const char *body, struct _http_header *headers, struct _http_connect *alive, 
	struct _http_response *rsp);
err_type http_response_parse(struct _http_response *rsp);
void http_response_clean(struct _http_response *rsp);
bool http_response_recv_finish(struct _http_response *rsp);
/* }-- public interface define */



#endif /* __AWOKE_HTTP_H__ */
