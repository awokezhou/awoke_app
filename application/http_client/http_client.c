
#include <getopt.h>

#include <stdio.h>

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "awoke_memory.h"

#include "http_client.h"
#include "mongoose.h"

#include "cJSON.h"

typedef enum {
	ARG_NONE = 0,
	ARG_SHOW_HEADERS,
	ARG_SHOW_BODYS,
} arg_opt;

void usage(int rc)
{
	printf("usage : http_client [option]\n\n");

    printf("    -u,\t\t--url\t\t\thttp url\n");
    printf("    -m,\t\t--method\t\thttp method, GET/POST\n");
	printf("    \t\t--show_headers\t\tshow http headers\n");

	EXIT(rc);	
}

static void parse_uri_component(const char **p, const char *end,
                                const char *seps, mem_ptr_t *res) 
{
	const char *q;
	res->p = *p;
	
	for (; *p < end; (*p)++) {
		for (q = seps; *q != '\0'; q++) {
			if (**p == *q) break;
		}
		if (*q != '\0') break;
	}
	
	res->len = (*p) - res->p;
	if (*p < end) (*p)++;
}

err_type http_parse_uri(const mem_ptr_t uri, mem_ptr_t *scheme,
					     mem_ptr_t *user_info, mem_ptr_t *host,
					     unsigned int *port, mem_ptr_t *path,
					     mem_ptr_t *query, mem_ptr_t *fragment)
{
	log_debug("http_parse_uri");
	unsigned int rport = 0;
	mem_ptr_t rscheme = mem_ptr_none();
	mem_ptr_t ruser_info = mem_ptr_none(); 
	mem_ptr_t rhost = mem_ptr_none();
	mem_ptr_t rpath = mem_ptr_none();
	mem_ptr_t rquery = mem_ptr_none();
	mem_ptr_t rfragment = mem_ptr_none();

	enum {
    	P_START,
    	P_SCHEME_OR_PORT,
    	P_USER_INFO,
    	P_HOST,
    	P_PORT,
    	P_REST
  	} state = P_START;
	
  	const char *p = uri.p, *end = p + uri.len;

	log_debug("uri len:%d", uri.len);

	while (p < end) {
		switch (state) {
			case P_START:
				/*
				* expecting on of:
				* - `scheme://xxxx`
				* - `xxxx:port`
				* - `[a:b:c]:port`
				* - `xxxx/path`
				*/
				if (*p == '[') {
					state = P_HOST;
					break;
				}
				for (; p < end; p++) {
					if (*p == ':') {
						state = P_SCHEME_OR_PORT;
						break;
					} else if (*p == '/') {
						state = P_REST;
						break;
					}
				}
				if (state == P_START || state == P_REST) {
					rhost.p = uri.p;
					rhost.len = p - uri.p;
				}
				break;
			case P_SCHEME_OR_PORT:
				if (end - p >= 3 && strncmp(p, "://", 3) == 0) {
					rscheme.p = uri.p;
					rscheme.len = p - uri.p;
					state = P_USER_INFO;
					p += 3;
				} else {
					rhost.p = uri.p;
					rhost.len = p - uri.p;
					state = P_PORT;
				}
				break;
			case P_USER_INFO:
				ruser_info.p = p;
				for (; p < end; p++) {
					if (*p == '@' || *p == '[' || *p == '/') {
						break;
					}
				}
				if (p == end || *p == '/' || *p == '[') {
					/* backtrack and parse as host */
					p = ruser_info.p;
				}
				ruser_info.len = p - ruser_info.p;
				state = P_HOST;
				break;
			case P_HOST:
				if (*p == '@') p++;
					rhost.p = p;
				if (*p == '[') {
					int found = 0;
					for (; !found && p < end; p++) {
						found = (*p == ']');
					}
					if (!found) return et_param;
				} else {
					for (; p < end; p++) {
						if (*p == ':' || *p == '/') break;
					}
				}
				rhost.len = p - rhost.p;
				if (p < end) {
					if (*p == ':') {
						state = P_PORT;
						break;
					} else if (*p == '/') {
						state = P_REST;
						break;
					}
				}
				break;
			case P_PORT:
				p++;
				for (; p < end; p++) {
					if (*p == '/') {
						state = P_REST;
						break;
					}
					rport *= 10;
					rport += *p - '0';
				}
				break;
			case P_REST:
				/* `p` points to separator. `path` includes the separator */
				parse_uri_component(&p, end, "?#", &rpath);
				if (p < end && *(p - 1) == '?') {
					parse_uri_component(&p, end, "#", &rquery);
				}
				parse_uri_component(&p, end, "", &rfragment);
				break;
		}
  	}

	if (scheme != 0) *scheme = rscheme;
	if (user_info != 0) *user_info = ruser_info;
	if (host != 0) *host = rhost;
	if (port != 0) *port = rport;
	if (path != 0) *path = rpath;
	if (query != 0) *query = rquery;
	if (fragment != 0) *fragment = rfragment;

	return et_ok;
}

#define mem_ptr_print(ptr) do {\
		fwrite(ptr.p, 1, ptr.len, stdout);\
		printf("\n");\
	}while(0);

err_type http_connect_create(http_connect *c, const char *uri)
{
	err_type ret;
	char addr[16] = {0x0};
	mem_ptr_t scheme, user, host, path, query, fragment;
	unsigned int d1,d2,d3,d4, port = 0;
	struct sockaddr_in sockaddr;

	if (!c)
		return et_param;

	ret = http_parse_uri(mem_mk_ptr((char *)uri), &scheme, &user, &host, 
						 &port, &path, &query, &fragment);
	c->uri = uri;
	c->host = host;
	c->path = path;
	c->user = user;
	c->query = query;
	c->scheme = scheme; 
	/*
	int fd;
	int ret;
	char _addr[16] = {0x0};

	fd = awoke_socket_create(AF_INET, SOCK_STREAM, 0);
	if (fd <= 0) {
		log_err("socket create error");
		return -1;
	}

	memcpy(_addr, addr.p, addr.len);

	c->sock = fd;
	c->addr.sin_family = AF_INET;
	c->addr.sin_addr.s_addr = inet_addr(_addr);
	c->addr.sin_port = htons(port);
	*/
	return 0;
}

int http_do_connect(http_conn *c)
{
	int ret;

	ret = connect(c->sock, (struct sockaddr*)&(c->addr), sizeof(c->addr));
	if (ret < 0) {
		log_err("connect error");
		close(c->sock);
		return -1;
	}

	return 0;
}

int http_connect_recv(http_conn *c, int timeout)
{
	int i;
	int rc = 0;
	bool ready;
	char buff[1024];
	
	struct timeval tm;
	tm.tv_sec = 1;
	setsockopt(c->sock, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm));

	//awoke_socket_set_timeout(c->sock, 1);

	for(i=0; i<timeout; i++) {
		rc = recv(c->sock, buff, sizeof(buff), MSG_WAITALL);
		log_info("buff len:%d\n%s", rc, buff);
		if (rc > 0) {
			return 0;
		}
	}

	return -1;
}

int http_connect_send(http_conn *c, http_request *r)
{
	int rc;
	int ret;

	rc = write(c->sock, r->original_buf, r->original_len);
	if (rc < 0) {
		if (errno == EPIPE) {
			return -1;
		} else {
			return -2;
		}
	} else if (rc != r->original_len) {
		return -3;
	}

	log_debug("send len:%d", rc);

	ret = http_connect_recv(c, 3);
	if (ret < 0)
		return -1;

	return 0;
}

typedef struct _header_entry {
	char* key;
	int type;
} header_entry;

static header_entry my_headers_table[] = 
{
	{"Accept:", 	HEADER_ACCEPT},
	{"Host:", HEADER_HOST},
	{"Accept-Charset:", HEADER_ACCEPT_CHARSET},
	{"Accept-Encoding:", HEADER_ACCEPT_ENCODING},
	{"Accept-Language:", HEADER_ACCEPT_LANGUAGE},
	{"Connection:", HEADER_CONNECTION},
	{"Content-Length:", HEADER_CONTENT_LENGTH},
	{"Content-Type:", HEADER_CONTENT_TYPE},
};
static int my_headers_size = array_size(my_headers_table);


	
#define headers_entry_foreach(head, size, p)		\
		int __i;									\
		p = &head[0];								\
													\
		for (__i = 0;								\
			 __i < size;							\
			 __i++, 								\
				p = &head[__i]) 					\


mem_ptr_t find_key(int key)
{	
	int size;
	header_entry *p;
	header_entry *header;
	mem_ptr_t ret = mem_ptr_none();

	size = my_headers_size;
	header = my_headers_table;
	
	headers_entry_foreach(header, size, p) {
		if (p->key == key)
			return mem_mk_ptr(p->key_p);
	}

	
	return ret;
}

void http_request_header(http_request *req, int key, mem_ptr_t val_p)
{
	http_header *h;

	mem_ptr_t key_p;

	key_p = find_key(key);

	h = &req->headers[key];
	if (h->type != key)
		h->type = key;
	mem_ptr_copy(&h->key, &key_p);
	mem_ptr_copy(&h->val, &val_p);
	log_debug("set header key:%.*s, val:%.*s", key_p.len, key_p.p, 
		val_p.len, val_p.p);
}

int _request_ori_build(http_request *req, const char *fmt, ...)
{
	int len;
	va_list ap;
	char *p = req->pos;
	int max = HTTP_BUFF_MAX - req->original_len;

	va_start(ap, fmt);
	len = vsnprintf(p, max, fmt, ap);
	max -= len;
	va_end(ap);

	req->original_len += len;
	req->pos+=len;
	
	return len;
}

void _build_header(http_request *req, http_header *h)
{
	_request_ori_build(req, "%.*s %.*s\r\n", h->key.len, h->key.p,
		h->val.len, h->val.p);
}

void _build_string(http_request *req, const char *string)
{
	int len;
	int max = HTTP_BUFF_MAX - req->original_len;
	char *p = req->pos;

	len = snprintf(p, max, "%s", string);
	req->original_len += len;
	req->pos+=len;
}

void http_request_build_headers(http_request *req)
{
	int key;
	http_header *h;
	
	for (key=0; key<HEADER_SIZEOF; key++) {
		h = &req->headers[key];
		log_debug("h type:%d, key:%.*s", h->type,
			h->key.len, h->key.p);
		if ((h->type == key) && (h->key.p != NULL)) {
			_build_header(req, h);
		}
	}
}

int http_request_build_startline(http_request *req)
{
	return _request_ori_build(req, "%.*s %.*s %.*s\r\n", req->method.len, req->method.p,
		req->path.len, req->path.p,
		req->protocol.len, req->protocol.p);
	/*
	len = snprintf(p, HTTP_BUFF_MAX, "%.*s %.*s %.*s\r\n",
				   req->method.len, req->method.p,
				   req->path.len, req->path.p,
				   req->protocol.len, req->protocol.p);
	req->original_len += len;
	log_info("start line:%.*s", len, p);
	req->pos+=len;*/
}

int http_request_build_body(http_request *req)
{
	/*cJSON *json_root, *json_msg;
	
	int type = 1;
	char *imei = "865484023020613";
	json_root = cJSON_CreateObject();
	unsigned long at = 1466133706;
	char *value = "505500CA1002000201000100B50186E";

	cJSON_AddItemToObject(json_root, "msg", json_msg = cJSON_CreateObject());	
	cJSON_AddNumberToObject(json_msg, "type", type);
	cJSON_AddStringToObject(json_msg, "imei", imei);
	cJSON_AddNumberToObject(json_msg, "at", at);
	cJSON_AddStringToObject(json_msg, "value", value);

	cJSON_AddNumberToObject(json_root, "data_src", 1);
	cJSON_AddNumberToObject(json_root, "ver", 0);
	cJSON_AddStringToObject(json_root, "msg_signature", "msg_signature");
	cJSON_AddStringToObject(json_root, "nonce", "abcdefgh");

	char *json_print = cJSON_Print(json_root);
	int json_len = strlen(json_print);
	log_debug("body len:%d", json_len);
	_request_ori_build(req, "%.*s", json_len, json_print);
	//log_debug("json:\n%s", json_print);
	cJSON_Delete(json_root);
	free(json_print);
	return json_len;*/
	return _request_ori_build(req, "%.*s", req->body.len, req->body.p);
}

static err_type http_post_request(const char *uri, cJSON *body, http_header *headers)
{
	err_type ret;
	http_request *req;
	cJSON *pbody = NULL;

	if (!uri || !body || !*body)
		return et_param;
	
	req = mem_alloc_z(sizeof(http_request));
	if (!req) {
		log_err("alloc request error");
		return et_nomem;
	}

	ret = http_connect_create(&req->conn, uri);
}

int http_package_request(http_request *req)
{
	req->pos = &req->original_buf;

	char content_length_str[16];
	
	http_request_build_startline(req);
	log_debug("req->original_len:%d", req->original_len);

	http_request_header(req, HEADER_HOST, mem_mk_ptr("47.99.107.151:8990"));
	sprintf(content_length_str, "%d", req->body.len);
	http_request_header(req, HEADER_CONTENT_LENGTH, mem_mk_ptr(content_length_str));
	//http_request_header(req, HEADER_CONTENT_TYPE, mem_mk_ptr("json"));

	http_request_build_headers(req);
	_build_string(req, "\r\n");

	http_request_build_body(req);
	log_debug("req->original_len:%d", req->original_len);

	log_info("pkt:\n%.*s", req->original_len, req->original_buf);
}

void http_connect_release(http_conn *c)
{
	if (c->sock)
		close(c->sock);
}

void cJSON_test()
{
	cJSON *json_root, *json_msg;

	int type = 1;
	char *imei = "865484023020613";
	json_root = cJSON_CreateObject();
	unsigned long at = 1466133706;
	char *value = "505500CA1002000201000100B50186E";

	cJSON_AddItemToObject(json_root, "msg", json_msg = cJSON_CreateObject());	
	cJSON_AddNumberToObject(json_msg, "type", type);
	cJSON_AddStringToObject(json_msg, "imei", imei);
	cJSON_AddNumberToObject(json_msg, "at", at);
	cJSON_AddStringToObject(json_msg, "value", value);

	cJSON_AddNumberToObject(json_root, "data_src", 1);
	cJSON_AddNumberToObject(json_root, "ver", 0);
	cJSON_AddStringToObject(json_root, "msg_signature", "msg_signature");
	cJSON_AddStringToObject(json_root, "nonce", "abcdefgh");

	char *json_print = cJSON_Print(json_root);
	log_debug("json:\n%s", json_print);
	cJSON_Delete(json_root);
	free(json_print);
}

void _http_client_test(const char *url, const char *body, const char *method)
{
	int ret;
	http_conn conn;
	http_request req;
	unsigned int port_i = 0;
	mem_ptr_t uri, scheme, query, fragment, user_info, host, path;
	
	log_debug("http_client_test");

	uri.p = url;
	uri.len = strlen(url);
	
	if (http_parse_uri(uri, &scheme, &user_info, &host, &port_i, &path,
		&query, &fragment) != 0) {
		log_err("parse error");
		return -1;
	}

	log_debug("scheme %.*s, user_info %.*s, host %.*s, path %.*s, query %.*s, fragment %.*s",
		scheme.len, scheme.p,
		user_info.len, user_info.p,
		host.len, host.p,
		path.len, path.p,
		query.len, query.p,
		fragment.len, fragment.p);

	req.method = mem_mk_ptr(method);
	req.path = path;
	req.protocol = mem_mk_ptr(HTTP_PROTOCOL_11_STR);
	req.body = mem_mk_ptr(body);

	ret = http_connect_create(&conn, host, port_i);
	if (ret < 0) {
		log_err("connect create error, ret %d", ret);
		return -1;
	}

	log_debug("connect create ok");

	ret = http_connect(&conn);
	if (ret < 0) {
		log_err("connect error, ret %d", ret);
		return -1;
	}
		
	log_debug("connect ok");

	ret = http_package_request(&req);
	if (ret < 0) {
		log_err("package error, ret %d", ret);
		return -1;
	}

	log_debug("package ok");
	/*
	ret = http_connect_send(&conn, &req);
	if (ret < 0) {
		log_err("send error %d", ret);
		return -1;
	}
*/
	log_debug("send ok");

	http_connect_release(&conn);

	return 0;
}

void http_client_test()
{
	cJSON *json_root, *json_msg;
	
	int type = 1;
	char *imei = "865484023020613";
	json_root = cJSON_CreateObject();
	char *value = "50550030100000080110001001B01845EC96880303"\
		          "07180150008000000000132648507A8E8525C0000000001845EBF0000000000";

	cJSON_AddItemToObject(json_root, "msg", json_msg = cJSON_CreateObject());	
	cJSON_AddNumberToObject(json_msg, "type", type);
	cJSON_AddStringToObject(json_msg, "imei", imei);
	cJSON_AddNumberToObject(json_msg, "at", 1466133706841);
	cJSON_AddStringToObject(json_msg, "value", value);

	cJSON_AddNumberToObject(json_root, "data_src", 1);
	cJSON_AddNumberToObject(json_root, "ver", 0);
	cJSON_AddStringToObject(json_root, "msg_signature", "message signature");
	cJSON_AddStringToObject(json_root, "nonce", "abcdefgh");

	char *mybpdy = cJSON_Print(json_root);
	//log_debug("json:\n%s", json_print);

	/*
	char *mybpdy = 	"{"\
					"\"at\":1561961036488,"\
					"\"imei\":\"860803031708510\","\
					"\"type\":1,"\
					"\"ds_id\":\"3336_0_5514\","\
					"\"value\":\"50550030100000080110001001B01845EC96880303"\
					"07180150008000000000132648507A8E8525C0000000001845EBF0000000000\","\
					"\"dev_id\":530711111"\
				"}";
				*/

	_http_client_test("http://www.szhqiot.top/nb/direct-connect/receive", 
			     mybpdy, 
			     METHOD_POST_STR);

	cJSON_Delete(json_root);
	free(mybpdy);
}

void http_request_build_test(http_request *req)
{
	req->method = mem_mk_ptr(METHOD_POST_STR);
	req->path = mem_mk_ptr("/nb/direct-connect/receive");
	req->protocol = mem_mk_ptr("HTTP/1.1");

	http_request_build_startline(req);
	log_debug("req->original_len:%d", req->original_len);

	http_request_header(req, HEADER_HOST, mem_mk_ptr("47.99.107.151"));
	http_request_header(req, HEADER_CONTENT_LENGTH, mem_mk_ptr("0"));
	http_request_header(req, HEADER_CONTENT_TYPE, mem_mk_ptr("json"));

	http_request_build_headers(req);
	log_debug("req->original_len:%d", req->original_len);

	log_info("pkt:\n%.*s", req->original_len, req->original_buf);
}

void http_request_test()
{
	http_request req;
	req.pos = &req.original_buf;
	http_request_build_test(&req);
	
}

static bool g_http_poll = TRUE;
static bool g_show_headers = FALSE;
static bool g_show_bodys = FALSE;
static char *g_url = "www.baidu.com";
static char *g_method = "GET";

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) 
{
	char buff[2048];
	 struct http_message *hm = (struct http_message *) ev_data;
	 
	 switch (ev) {
	 	
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0) {
				g_http_poll = false;
			}
			break;
			
		case MG_EV_HTTP_REPLY:
			nc->flags |= MG_F_CLOSE_IMMEDIATELY;
			log_info("HTTP REPLY:");
			if (g_show_headers) {
				fwrite(hm->message.p, 1, hm->message.len, stdout);
			} else {
				fwrite(hm->body.p, 1, hm->body.len, stdout);
			}
			g_http_poll = false;
			break;
			
		case MG_EV_CLOSE:
			log_debug("MG_EV_CLOSE");
			if (!g_http_poll) {
				log_debug("Server closed connection\n");
				g_http_poll = false;
			}
			break;
		default:
			break;
	 }
}

void mongoose_test(int argc, char **argv)
{
	log_debug("mg mgr init");

	int opt;
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, NULL);
	
	static const struct option long_opts[] = {
        {"show-headers",	no_argument,        NULL,   ARG_SHOW_HEADERS},
        {"show-bodys",		no_argument,		NULL,	ARG_SHOW_BODYS},
        {"url",   			required_argument,  NULL,   'u'},
        {"method",			required_argument, 	NULL,   'm'},
        {NULL, 0, NULL, 0}
    };

	while ((opt = getopt_long(argc, argv, "u:m:?h", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case ARG_SHOW_HEADERS:
                g_show_headers = TRUE;
                break;

			case ARG_SHOW_BODYS:
				g_show_bodys = TRUE;
				break;
				
            case 'u':
                g_url = optarg;
                break;

			case 'm':
				g_method = optarg;
				break;
				
            case '?':
            case 'h':
            default:
                usage(AWOKE_EXIT_SUCCESS);
        }
    }

	log_info("url %s", g_url);
	log_info("method %s", g_method);

	cJSON *json_root, *json_msg;
	int type = 1;
	char *imei = "865484023020613";
	json_root = cJSON_CreateObject();
	char *value = "50550030100000080110001001B01845EC96880303"\
		          "07180150008000000000132648507A8E8525C0000000001845EBF0000000000";

	cJSON_AddItemToObject(json_root, "msg", json_msg = cJSON_CreateObject());	
	cJSON_AddNumberToObject(json_msg, "type", type);
	cJSON_AddStringToObject(json_msg, "imei", imei);
	cJSON_AddNumberToObject(json_msg, "at", 1466133706841);
	cJSON_AddStringToObject(json_msg, "value", value);

	cJSON_AddNumberToObject(json_root, "data_src", 1);
	cJSON_AddNumberToObject(json_root, "ver", 0);
	cJSON_AddStringToObject(json_root, "msg_signature", "message signature");
	cJSON_AddStringToObject(json_root, "nonce", "abcdefgh");

	char *mybpdy = cJSON_Print(json_root);
	/*char *mybpdy = 		"{"\
							"\"at\":1561961036488232,"\
							"\"imei\":\"860803031708510\","\
							"\"type\":1,"\
							"\"ds_id\":\"3336_0_5514\","\
							"\"value\":\"50550030100000080110001001B01845EC96880303"\
							"07180150008000000000132648507A8E8525C0000000001845EBF0000000000\","\
							"\"dev_id\":530711111"\
						"}";*/
	
	mg_connect_http(&mgr, ev_handler, g_url, NULL, mybpdy);

	while (g_http_poll) {
		log_debug("polling");
    	mg_mgr_poll(&mgr, 1000);
  	}
  	mg_mgr_free(&mgr);

	log_debug("mgr free");
}

static mem_ptr_t smp_mkptr_num(int x)
{
	char num[16] = {0x0};

	sprintf(num, "%d", x);
	return mem_mk_ptr(num);
}

void mem_ptr_test()
{
	mem_ptr_t test = smp_mkptr_num(35);
	log_debug("test:%.*s", test.len, test.p);
}

int main(int argc, char **argv)
{
	log_mode(LOG_TEST);
	log_level(LOG_DBG);

	//mongoose_test(argc, argv);

	http_request_test();

	//log_debug("will http_client_test");

	//http_client_test();

	//cJSON_test();

	//mem_ptr_test();
	
	return 0;
}
