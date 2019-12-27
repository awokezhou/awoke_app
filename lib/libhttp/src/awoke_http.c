
#include "awoke_http.h"
#include "awoke_log.h"



/*
-- Module Description
  This is a custom HTTP module.

-- Commit Information --
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   ci       |  date       |  version |  description
 -------------------------------------------------------------------------
   610819fe |  2019-12-24 |  0.0.1   |  create libhttp
            |             |			 |
   9de45551 |  2019-12-25 |  0.0.2   |  support buffchunk, response, add
   			|			  |  		 |  loop receive and send
   			|			  |			 |	
   4294bcf0 |  2019-12-27 |  0.0.3	 |  memory test for buffchunk; tcp 
   										connect test for conn-keep
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/



static err_type http_response_preparse(struct _http_response *rsp);




static http_header_entry g_header_keys_table[] = 
{
	http_header_entry_make(HTTP_HEADER_HOST),
	http_header_entry_make(HTTP_HEADER_USER_AGENT),
	http_header_entry_make(HTTP_HEADER_ACCEPT),
	http_header_entry_make(HTTP_HEADER_ACCEPT_CHARSET),
	http_header_entry_make(HTTP_HEADER_ACCEPT_ENCODING),
	http_header_entry_make(HTTP_HEADER_ACCEPT_LANGUAGE),
	http_header_entry_make(HTTP_HEADER_CONNECTION),
	http_header_entry_make(HTTP_HEADER_CONTENT_LENGTH),
	http_header_entry_make(HTTP_HEADER_CONTENT_TYPE),
	http_header_entry_make(HTTP_HEADER_KEEP_ALIVE),
	http_header_entry_make(HTTP_HEADER_AUTHORIZATION),
};

static http_header_entry g_protocol_table[] = 
{
	http_header_entry_make(HTTP_PROTOCOL_10),
	http_header_entry_make(HTTP_PROTOCOL_11),
	http_header_entry_make(HTTP_PROTOCOL_20),
};

static mem_ptr_t find_key(int type)
{	
	int size;
	http_header_entry *p;
	http_header_entry *header;

	header = g_header_keys_table;
	size = array_size(g_header_keys_table);
	
	array_foreach(header, size, p) {
		if (p->type == type)
			return mem_mk_ptr(p->key);
	}

	return mem_mk_ptr(HTTP_HEADER_UNKNOWN_STR);
}

static int parse_key(mem_ptr_t key)
{
	int size;
	http_header_entry *p;
	http_header_entry *header;	

	header = g_header_keys_table;
	size = array_size(g_header_keys_table);
	
	array_foreach(header, size, p) {
		if (!strncmp(p->key, key.p, key.len)) {
			return p->type;
		}
	}

	return HTTP_HEADER_UNKNOWN;
}

static int get_protocol(mem_ptr_t proto_p)
{
	int size;
	http_header_entry *p;
	http_header_entry *header;

	header = g_protocol_table;
	size = array_size(g_protocol_table);

	array_foreach(header, size, p) {
		if (!strncmp(p->key, proto_p.p, proto_p.len))
			return p->type;
	}

	return HTTP_PROTOCOL_UNKNOWN;
}

static void parse_uri_component(const char **p, const char *end,
                                		const char *seps, mem_ptr_t *res) 
{
	const char *q;
	res->p = (char *)*p;
	
	for (; *p < end; (*p)++) {
		for (q = seps; *q != '\0'; q++) {
			if (**p == *q) break;
		}
		if (*q != '\0') break;
	}
	
	res->len = (*p) - res->p;
	if (*p < end) (*p)++;
} 

static err_type uri_parse(const mem_ptr_t uri, mem_ptr_t *scheme, mem_ptr_t *user, 
	mem_ptr_t *host, unsigned int *port, mem_ptr_t *path, 
	mem_ptr_t *query, mem_ptr_t *fragment)
{
	unsigned int rport = 0;
	mem_ptr_t rscheme		= mem_ptr_none();
	mem_ptr_t ruser_info	= mem_ptr_none(); 
	mem_ptr_t rhost 		= mem_ptr_none();
	mem_ptr_t rpath 		= mem_ptr_none();
	mem_ptr_t rquery		= mem_ptr_none();
	mem_ptr_t rfragment 	= mem_ptr_none();	

	enum {
		P_START,
		P_SCHEME_OR_PORT,
		P_USER_INFO,
		P_HOST,
		P_PORT,
		P_REST
	} state = P_START;

	const char *p = uri.p, *end = p + uri.len;

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
				ruser_info.p = (char *)p;
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
					rhost.p = (char *)p;
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

	if (host != 0) *host = rhost;
	if (port != 0) *port = rport;
	if (path != 0) *path = rpath;
	if (query != 0) *query = rquery;
	if (user != 0) *user = ruser_info;
	if (scheme != 0) *scheme = rscheme; 
	if (fragment != 0) *fragment = rfragment;

	return et_ok;
}

static err_type get_host_address(char *hostname, struct sockaddr_in *addr)
{
	struct hostent *host;

	host = gethostbyname(hostname);
	if (!host) {
		log_err("gethostbyname error");
		return et_fail;
	}

	memcpy(&addr->sin_addr.s_addr, host->h_addr_list[0], 4);
	return et_ok;
}

bool http_buffchunk_dynamic(struct _http_buffchunk *chunk)
{
	return ((chunk->buff) && (chunk->buff != chunk->_fixed));
}

void http_buffchunk_dump(struct _http_buffchunk *chunk)
{
	log_test(">>> buffchunk dump:");
	log_test("------------------------");
	log_test("size:%d", chunk->size);
	log_test("length:%d", chunk->length);
	log_test("buff address:0x%x", chunk->buff);
	log_test("fixed address:0x%x", chunk->_fixed);
	log_test("------------------------");
}

void http_buffchunk_copy(struct _http_buffchunk *dst, struct _http_buffchunk *src)
{
	dst->size = src->size;
	dst->length = src->length;

	/*
	 * Buffchunk copy need to be careful. If it's a dynamic buffchunk, 
	 * only point the dst buffchunk's "buff" field to the "buff" field 
	 * of the src buffchunk. If still using fixed buff, need to copy 
	 * "_fixed" field to dst buffchunk, and point the dst buffchunk's
	 * "buff" field to it self's "_fixed" field.
	 */

	if (http_buffchunk_dynamic(src)) {
		dst->buff = src->buff;
		return;
	}

	memcpy(dst->_fixed, src->_fixed, src->length);
	dst->buff = dst->_fixed;
}

err_type http_buffchunk_resize(struct _http_buffchunk *chunk, int new_size)
{
	char *realloc = NULL;
	
	if (chunk->size >= new_size) 
		return et_ok;

	if (new_size > HTTP_BUFFER_LIMIT)
		return et_mem_limit;
	
	if (!http_buffchunk_dynamic(chunk)) {
		log_test("buffchunk not dynamic, alloc memory");
		chunk->buff = mem_alloc_z(new_size+1);
		if (!chunk->buff) {
			log_err("alloc chunk error");
			return et_nomem;
		}
		
		memcpy(chunk->buff, chunk->_fixed, chunk->length);
	
	} else {
		log_test("buffchunk dynamic, realloc memory");
		realloc = mem_realloc(chunk->buff, new_size+1);
		if (!chunk->buff) {
			log_err("realloc chunk error");
			return et_nomem;
		}

		chunk->buff = realloc;
	}

	chunk->size = new_size;

	return et_ok;
}

static void header_set(struct _http_header *headers, int type, mem_ptr_t val)
{
	struct _http_header *h;

	mem_ptr_t key = find_key(type);
	if (!key.p) 
		return;
	
	h = &headers[type];
	if (h->type != type)
		h->type = type;
	
	mem_ptr_copy(&h->key, &key);
	mem_ptr_copy(&h->val, &val);
}

static void request_set_header(struct _http_request *req, int type, mem_ptr_t val)
{
	return header_set(req->headers, type, val);
}

static void request_set_extend_header(struct _http_request *req)
{
	struct _http_header *h;
	struct _http_header *extend = req->extend_headers;

	array_foreach(extend, HTTP_HEADER_SIZEOF, h) {
		if (h->type != HTTP_HEADER_UNKNOWN) {
			request_set_header(req, h->type, h->val);
		}
	}

	return;
}

void http_header_set(struct _http_header *headers, int type, char *value)
{
	mem_ptr_t val;

	if (!value)
		return;

	val = mem_mk_ptr(value);

	return header_set(headers, type, val);
}

void http_header_init(struct _http_header *headers)
{
	struct _http_header *h;

	array_foreach(headers, HTTP_HEADER_SIZEOF, h) {
		h->key = mem_mk_ptr(NULL);
		h->val = mem_mk_ptr(NULL);
		h->type = HTTP_HEADER_UNKNOWN;
	}
}

static int request_build_header(struct _http_request *req, struct _http_header *h)
{
  	return build_ptr_format(req->bp, "%.*s: %.*s"STR_CRLD, 
							h->key.len, h->key.p,
							h->val.len, h->val.p);
}

static int request_build_string(struct _http_request *req, const char *string)
{
	return build_ptr_string(req->bp, string);
}

static int request_build_body(struct _http_request *req)
{
	return build_ptr_format(req->bp, "%.*s", req->body.len, req->body.p);
}

static int request_build_startline(struct _http_request *req)
{	
	return build_ptr_format(req->bp, "%.*s %.*s %.*s"STR_CRLD,
					 	    req->method.len, req->method.p,
					 		req->path.len, req->path.p,
					 		req->protocol.len, req->protocol.p);
}

static int request_build_headers(struct _http_request *req)
{
	int len;
	struct _http_header *h;

	array_foreach(req->headers, HTTP_HEADER_SIZEOF, h) {
		if (h->key.p) {
			len += request_build_header(req, h);
		}
	}

	return len;
}

void http_request_clean(struct _http_request *p)
{
	if (!http_connect_keep_alive(&p->conn)) {
		log_test("connect not keep alive, will release");
		http_connect_release(&p->conn);
	}

	if (http_buffchunk_dynamic(&p->buffchunk))
		mem_free(&p->buffchunk);
}

void http_request_dump(struct _http_request *req)
{
	struct _http_header *h;
	
	log_info(">>> HTTP Request dump:");
	log_info("--------------------------------");
	log_info("uri : %s", req->uri);
	log_info("method : %.*s", req->method.len, req->method.p);
	log_info("protocol : %.*s", req->protocol.len, req->protocol.p);
	array_foreach(req->headers, HTTP_HEADER_SIZEOF, h) {
		if (h->key.len) {
			log_info("%.*s %.*s", h->key.len, h->key.p,
				h->val.len, h->val.p);
		}
	}
	log_test("HTTP Text:\n%.*s", req->buffchunk.length, req->buffchunk.buff);
	log_info("--------------------------------");
}

void http_request_init(struct _http_request *req)
{
	req->uri = NULL;
	
	req->body = mem_mk_ptr(NULL);
	req->host = mem_mk_ptr(NULL);
	req->path = mem_mk_ptr(NULL);
	req->method = mem_mk_ptr(NULL);
	req->protocol = mem_mk_ptr(NULL);

	http_buffchunk_init(req->buffchunk);
	
	req->bp = build_ptr_make(req->buffchunk.buff, req->buffchunk.size);
}

static err_type http_request_package(struct _http_request *req)
{
	request_set_header(req, HTTP_HEADER_HOST, req->host);

	if (req->body.len)
		request_set_header(req, HTTP_HEADER_CONTENT_LENGTH, mem_mkptr_num(req->body.len));

	if (req->extend_headers)
		request_set_extend_header(req);
	
	request_build_startline(req);
	
	request_build_headers(req);
	
	request_build_string(req, STR_CRLD);

	if (req->body.len) {
		request_build_body(req);
	}

	req->buffchunk.length = req->bp.len;

	http_request_dump(req);

	return et_ok;
}

bool http_reqeust_send_finish(struct _http_request *req, int send_total)
{
	return (req->buffchunk.length == send_total);
}

err_type http_request_send(struct _http_request *req)
{
	err_type ret;
	int max_send;
	int send_size = 0;
	int send_total = 0;
	struct _http_buffchunk *chunk = &req->buffchunk;
	struct _awoke_tcp_connect *conn = &req->conn._conn;

	do {

		max_send = chunk->length - send_total;
		max_send = max_send > HTTP_BUFFER_CHUNK ? HTTP_BUFFER_CHUNK : max_send;
		log_test("max_send %d", max_send);
		
		ret = awoke_tcp_connect_send(conn, chunk->buff+send_size, max_send, 
									 &send_size);
		if (ret != et_ok) {
			log_err("connect send error, ret %d", ret);
			goto out;
		}

		send_total += send_size;
		log_test("send_size %d send_total %d", send_size, send_total);
		
	} while (!http_reqeust_send_finish(req, send_total));
	
out:
	return ret;
}

err_type http_response_recv(struct _http_request *req, struct _http_response *rsp)
{
	err_type ret;
	int max_read;
	int new_size;
	int read_size;
	struct _http_buffchunk *chunk;
	struct _awoke_tcp_connect *conn = &req->conn._conn;

	chunk = &rsp->buffchunk;
	
	do {

		if (!chunk->length) {
			log_test("first read");
			/* first time receive */
		
		} else {

			new_size = chunk->size + HTTP_BUFFER_CHUNK;
			log_test("new size %d", new_size);
			ret = http_buffchunk_resize(chunk, new_size);
			if (ret != et_ok) {
				log_err("buffchunk resize error, ret %d", ret);
				goto recv_error;
			}

			//chunk->size = new_size;
			log_test("buffchunk size %d", chunk->size);
		}

		max_read = chunk->size - chunk->length;
		log_test("max read %d", max_read);
		ret = awoke_tcp_connect_recv(conn, chunk->buff+chunk->length, max_read, 
									 &read_size, FALSE);
		if (ret != et_ok) {
			log_err("receive error, ret %d", ret);
			goto recv_error;
		}

		log_test("tcp connect read %d", read_size);

		chunk->length += read_size;
		chunk->buff[chunk->length] = '\0';
		
	} while (!http_response_recv_finish(rsp));

	http_response_parse(rsp);
	return et_ok;
	
recv_error:
	if (http_buffchunk_dynamic(chunk))
		mem_free(chunk->buff);
	mem_free(rsp);
	return ret;
}

static err_type response_parse_header(struct _http_response *rsp)
{
	char *pos;
	char *end;
	char *head;
	char *tail;
	mem_ptr_t header_key;
	mem_ptr_t header_val;

	head = rsp->buffchunk.buff;
	tail = strstr(head, "\r\n");
	if (!tail) {
		log_err("parse startline error");
		return et_parser;
	}

	rsp->firstline.p = head;
	rsp->firstline.len = tail-head;

	log_test("find startline, head(%d):%.*s", rsp->firstline.len, 
		rsp->firstline.len, rsp->firstline.p);

	pos = strstr(head, " ");
	if (!pos) {
		log_err("parse startline error");
		return et_parser;
	}
	rsp->protocol_p.p = head;
	rsp->protocol_p.len = pos-head;
	log_test("protocol:%.*s", rsp->protocol_p.len, rsp->protocol_p.p);
	rsp->protocol = get_protocol(rsp->protocol_p);
	log_test("protocol:%d", rsp->protocol);
		
	head = pos + 1;
	pos = strstr(head, " ");
	if (!pos) {
		log_err("parse startline error");
		return et_parser;
	}
	rsp->statuscode_p.p = head;
	rsp->statuscode_p.len = tail-head;
	log_test("statuscode:%.*s", rsp->statuscode_p.len, rsp->statuscode_p.p);
	sscanf(head, "%d %*s", &rsp->status);
	log_test("status:%d", rsp->status);

	head = tail + 2;
	tail = strstr(head, "\r\n\r\n");
	if (!tail) {
		log_err("parse headers error");
		return et_parser;
	}
	
	while ((end = strstr(head, "\r\n")) && (end <= tail)) {

		pos = strstr(head, ": ");
		header_key.p = head;
		header_key.len = pos-head;
		head = pos + 2;
		header_val.p = head;
		header_val.len = end-head;

		header_set(rsp->headers, parse_key(header_key), header_val);
			
		head = end + 2;
	}

	return et_ok;
}

static void response_copy(struct _http_response *dst, struct _http_response *src)
{
	dst->status = src->status;
	dst->protocol = src->protocol;
	dst->connection = src->connection;
	dst->content_length = src->content_length;

	mem_ptr_copy(&dst->firstline, &src->firstline);
	mem_ptr_copy(&dst->protocol_p, &src->protocol_p);
	mem_ptr_copy(&dst->statuscode_p, &src->statuscode_p);
	mem_ptr_copy(&dst->body, &src->body);
	
	http_buffchunk_copy(&dst->buffchunk, &src->buffchunk);
}

void http_response_init(struct _http_response *rsp)
{
	rsp->status = HTTP_CODE_SUCCESS;
	rsp->content_length = 0;
	rsp->connection = 0;
	rsp->protocol = HTTP_PROTOCOL_UNKNOWN;
	
	rsp->firstline = mem_mk_ptr(NULL);
	rsp->protocol_p = mem_mk_ptr(NULL);
	rsp->statuscode_p = mem_mk_ptr(NULL);
	rsp->body = mem_mk_ptr(NULL);

	http_header_init(rsp->headers);
		
	http_buffchunk_init(rsp->buffchunk);
}

void http_response_clean(struct _http_response *rsp)
{
	if (http_buffchunk_dynamic(&rsp->buffchunk)) {
		log_test("buffchunk dynamic");
		mem_free(rsp->buffchunk.buff);
	}
}

bool http_response_recv_finish(struct _http_response *rsp)
{
	http_response_preparse(rsp);

	log_test("bodylen:%d", rsp->body.len);

	if (rsp->body.len == rsp->content_length) 
		return TRUE;

	return FALSE;
}

void http_response_dump(struct _http_response *rsp)
{
	struct _http_header *h;
	
	log_info(">> HTTP Response dump:");
	log_info("--------------------------------");
	log_info("status : %d", rsp->status);
	log_info("protocol: %.*s", rsp->protocol_p.len, rsp->protocol_p.p);
	log_info("content length : %d", rsp->content_length);
	array_foreach(rsp->headers, HTTP_HEADER_SIZEOF, h) {
		if (h->key.len) {
			log_info("%.*s %.*s", h->key.len, h->key.p,
				h->val.len, h->val.p);
		}
	}
	log_test("HTTP Text:\n%.*s", rsp->buffchunk.length, rsp->buffchunk.buff);
	log_info("--------------------------------");
}

err_type http_response_parse(struct _http_response *rsp)
{
	return response_parse_header(rsp);
}

static err_type http_response_preparse(struct _http_response *rsp)
{
	char *pos;
	http_buffchunk *chunk = &rsp->buffchunk;

	if (!rsp->content_length) {
		pos = strstr(chunk->buff, "Content-Length");
		if (pos) {
			sscanf(pos, "Content-Length: %lu\r\n", &rsp->content_length);
			log_debug("content length:%lu", rsp->content_length);
		}
	}

	pos = strstr(chunk->buff, "\r\n\r\n");
	pos += 4;
	rsp->body.p = pos;
	rsp->body.len = &chunk->buff[chunk->length] - pos;

	return et_ok;
}

static err_type http_connect_create(struct _http_connect *c, const char *uri)
{
	err_type ret;
	char addr[16] = {0x0};
	struct sockaddr_in sockaddr;
	unsigned int d1,d2,d3,d4, port = 0;
	mem_ptr_t scheme, user, host, path, query, fragment;

	if (!c) 
		return et_param;

	ret = uri_parse(mem_mk_ptr((char *)uri), &scheme, &user, 
					&host, &port, &path, &query, &fragment);
	if (ret != et_ok) 
		return ret;

	if (!path.len)
		path = mem_mk_ptr("/");
	
	c->uri = uri;
	c->host = host;
	c->path = path;
	c->user = user;
	c->query = query;
	c->scheme = scheme; 

	if (port==0) port=80;

	if (sscanf(host.p, "%u.%u.%u.%u", &d1, &d2, &d3, &d4) != 4) {
		memset(&sockaddr, 0x0, sizeof(sockaddr));
		sprintf(c->hostname, "%.*s", (int)host.len, host.p);
		ret = get_host_address(c->hostname, &sockaddr);
		if (ret != et_ok) {
			log_err("dns parse error, ret %d", ret);
			return ret;
		}
		sprintf(addr, "%s", inet_ntoa(sockaddr.sin_addr));
		mask_push(c->flag, HTTP_CONN_F_HOSTNAME);
	} else {
		memcpy(addr, host.p, host.len);
	}

	log_debug("address:%s, port:%d", addr, port);

	ret = awoke_tcp_connect_create(&c->_conn, addr, port);
	if (ret != et_ok) {
		log_err("tcp connect create error, ret %d", ret);
		return ret;
	}

	return et_ok;
}

void http_connect_dump(struct _http_connect *c)
{
	struct _awoke_tcp_connect *tcp_conn = &c->_conn;
	
	log_test(">>> connect dump:");
	log_test("------------------------");
	log_test("address:%s", inet_ntoa(tcp_conn->addr.sin_addr));
	log_test("port:%d", htons(tcp_conn->addr.sin_port));
	log_test("socket:%d", tcp_conn->sock);
	log_test("status:0x%x", tcp_conn->status);
	log_test("------------------------");
}

void http_connect_release(struct _http_connect *c)
{
	awoke_tcp_connect_release(&c->_conn);
}

err_type http_do_connect(struct _http_connect *c)
{
	return awoke_tcp_do_connect(&c->_conn);
}

bool http_connect_keep_alive(struct _http_connect *c)
{
	return mask_exst(c->flag, HTTP_CONN_F_KEEP_ALIVE);
}

void http_connect_set_keep_alive(struct _http_connect *c)
{
	mask_push(c->flag, HTTP_CONN_F_KEEP_ALIVE);
}

/*
 * Build a Http request, send to server and receive response.
 * @uri: request url
 * @method: GET/POST
 * @protocol: HTTP/1.0 HTTP/1.1
 * @body: HTTP body
 * @headers: extend HTTP header array
 * @alive: use for keep-alive connect. If not set, connect will be released when request over.
 *		   If set, connect will copy to this param
 * @rsp: the response from server
 */
err_type http_request(const char *uri, const char *method, const char *protocol, 
	const char *body, struct _http_header *headers, struct _http_connect *alive, 
	struct _http_response *rsp)
{
	err_type ret;
	struct _http_request request;
	struct _http_response response;

	if (!uri || !method || !protocol)
		return et_param;

	http_request_init(&request);

	/* 
	 * Alive connect is support for connect import or keep-alive. If alive connect
	 * exist and is connected, this is a import connect, and we should copy
	 * to req's conn.
	 */
	if (alive && alive->_conn.status == tcp_connected) {
		log_debug("import connect active, don't need to create one");
		memcpy(&request.conn, alive, sizeof(struct _http_connect));
		http_connect_dump(&request.conn);
		goto connected;		
	} 

	if (alive) {
		log_debug("connect need keep alive");
		http_connect_set_keep_alive(&request.conn);
	}

	ret = http_connect_create(&request.conn, uri);
	if (ret != et_ok) {
		log_err("connect create error, ret %d", ret);
		goto release;
	}
	
	ret = http_do_connect(&request.conn);
	if (ret != et_ok) {
		log_err("do connect error, ret %d", ret);
		goto release;
	}

connected:
	log_test("connected");
	request.uri = request.conn.uri;
	request.extend_headers = headers;
	request.method = mem_mk_ptr(method);
	request.protocol = mem_mk_ptr(protocol);
	if (!strncmp(HTTP_PROTOCOL_11_STR, request.protocol.p, request.protocol.len)) {
		request_set_header(&request, HTTP_HEADER_CONNECTION, mem_mk_ptr("keep-alive"));
	}
	
	mem_ptr_copy(&request.host, &request.conn.host);
	mem_ptr_copy(&request.path, &request.conn.path);
	if (!strncmp(HTTP_METHOD_POST_STR, request.method.p, request.method.len)) {
		request.body = mem_mk_ptr(body);
		http_buffchunk_resize(&request.buffchunk, request.body.len);
	}

	ret = http_request_package(&request);
	if (ret != et_ok) {
		log_err("package post request error");
		goto release;
	}

	ret = http_request_send(&request);
	if (ret != et_ok) {
		log_err("request send error, ret %d", ret);
		goto release;
	}

	log_test("request send ok");

	http_response_init(&response);
	ret = http_response_recv(&request, &response);
	if (ret != et_ok) {
		log_err("receive error, ret %d", ret);
		goto release;
	}

	log_test("response receive ok");

	http_response_dump(&response);
	
	if (rsp) {
		log_test("response need copy");
		response_copy(rsp, &response);
	}

	if (alive) {
		memcpy(alive, &request.conn, sizeof(struct _http_connect));
	}
	
release:
	http_request_clean(&request);
	log_test("http_request_clean ok");
	if (!rsp) {
		log_test("!rsp");
		http_response_clean(&response);
	}
	return ret;
}
