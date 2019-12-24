
#include "awoke_http.h"
#include "awoke_log.h"



static http_header_entry g_header_keys_table[] = 
{
	http_header_entry_make(HTTP_HEADER_ACCEPT),
	http_header_entry_make(HTTP_HEADER_HOST),
	http_header_entry_make(HTTP_HEADER_ACCEPT_CHARSET),
	http_header_entry_make(HTTP_HEADER_ACCEPT_ENCODING),
	http_header_entry_make(HTTP_HEADER_ACCEPT_LANGUAGE),
	http_header_entry_make(HTTP_HEADER_CONNECTION),
	http_header_entry_make(HTTP_HEADER_CONTENT_LENGTH),
	http_header_entry_make(HTTP_HEADER_CONTENT_TYPE),
	http_header_entry_make(HTTP_HEADER_USER_AGENT),
	http_header_entry_make(HTTP_HEADER_KEEP_ALIVE),
	http_header_entry_make(HTTP_HEADER_AUTHORIZATION),
};

static mem_ptr_t find_key(int type)
{	
	int size;
	http_header_entry *p;
	http_header_entry *header;
	mem_ptr_t ret = mem_ptr_none();

	header = g_header_keys_table;
	size = array_size(g_header_keys_table);
	
	array_foreach(header, size, p) {
		if (p->type == type)
			return mem_mk_ptr(p->key);
	}

	return ret;
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

static void header_set(struct _http_header *headers, int type, mem_ptr_t val)
{
	struct _http_header *h;

	mem_ptr_t key = find_key(type);
	if (!key.p) 
		return;

	log_debug("find key %.*s", key.len, key.p);
	
	h = &headers[type];
	log_debug("h type %d", h->type);
	if (h->type != type)
		h->type = type;
	
	mem_ptr_copy(&h->key, &key);
	mem_ptr_copy(&h->val, &val);
}

static void request_set_header(struct _http_request *req, int type, mem_ptr_t val)
{
	log_debug("header:type %d val %.*s", type, val.len, val.p);
	return header_set(req->headers, type, val);
}

static void request_header_dump(struct _http_request *req)
{
	struct _http_header *h;

	log_info(">>> headers:");

	array_foreach(req->headers, HTTP_HEADER_SIZEOF, h) {
		if (h->key.len) {
			log_info("\t%.*s%.*s", h->key.len, h->key.p,
				h->val.len, h->val.p);
		}
	}
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

static int request_build_header(struct _http_request *req, struct _http_header *h)
{
  	return build_ptr_format(req->bp, "%.*s %.*s\r\n", 
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
	return build_ptr_format(req->bp, "%.*s %.*s %.*s\r\n",
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

void http_request_clean(struct _http_request **req)
{
	struct _http_request *p;

	if (!req || !*req)
		return;

	p = *req;

	http_connect_release(&p->conn);

	mem_free(p);
	p = NULL;
}

void http_request_init(struct _http_request *req)
{
	req->uri = NULL;
	
	req->body = mem_mk_ptr(NULL);
	req->host = mem_mk_ptr(NULL);
	req->path = mem_mk_ptr(NULL);
	req->method = mem_mk_ptr(NULL);
	req->protocol = mem_mk_ptr(NULL);

	req->original_len = 0;
	req->bp = build_ptr_make(req->original_buf, HTTP_REQPKT_SIZE);
}

static err_type http_request_package(struct _http_request *req)
{
	request_set_header(req, HTTP_HEADER_HOST, req->host);

	if (req->body.len)
		request_set_header(req, HTTP_HEADER_CONTENT_LENGTH, mem_mkptr_num(req->body.len));

	if (req->extend_headers)
		request_set_extend_header(req);

	log_info("method:%.*s", req->method.len, req->method.p);
	log_info("body:%.*s", req->body.len, req->body.p);
	log_info("protocol:%.*s", req->protocol.len, req->protocol.p);
	request_header_dump(req);
	
	request_build_startline(req);
	
	request_build_headers(req);
	
	request_build_string(req, STR_CRLD);
	
	request_build_body(req);

	req->original_len = req->bp.len;
	
	log_debug(">>> HTTP Packet:\n%.*s", req->bp.len, req->bp.head);

	return et_ok;
}

err_type http_request_send(struct _http_request *req)
{
	return awoke_tcp_connect_send(&req->conn._conn, (void *)&req->original_buf, 
							     req->original_len);
}

err_type http_request_recv(struct _http_request *req)
{
	memset(&req->original_buf, 0x0, HTTP_REQPKT_SIZE);
	return awoke_tcp_connect_recv(&req->conn._conn, (void *)&req->original_buf, 
								  HTTP_REQPKT_SIZE);
}

static void http_state_code(struct _http_request *r)
{
	char *search = NULL;

	search = strstr(r->original_buf, HTTP_PROTOCOL_11_STR);
	if (!search) {
		search = strstr(r->original_buf, HTTP_PROTOCOL_10_STR);
		if (!search) {
			log_err("unknown version");
			/* unknown version also use default format */
		} 
		search += strlen(HTTP_PROTOCOL_10_STR);
		sscanf(search, " %d[^ ]", &r->state_code);
	} else {
		search += strlen(HTTP_PROTOCOL_11_STR);
		sscanf(search, " %d[^ ]", &r->state_code);
	}
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

	log_info("uri:%s", uri);
	log_info("host:%.*s", host.len, host.p);
	log_info("path:%.*s", path.len, path.p);
	log_info("user:%.*s", user.len, user.p);
	log_info("query:%.*s", query.len, query.p);
	log_info("scheme:%.*s", scheme.len, scheme.p);

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
		log_info("host address %s", addr);
		c->flag = HTTP_CONN_F_HOSTNAME;
	} else {
		memcpy(addr, host.p, host.len);
	}

	ret = awoke_tcp_connect_create(&c->_conn, addr, port);
	if (ret != et_ok) {
		log_err("tcp connect create error, ret %d", ret);
		return ret;
	}

	return et_ok;
}

void http_connect_release(struct _http_connect *c)
{
	awoke_tcp_connect_release(&c->_conn);
}

err_type http_do_connect(struct _http_connect *c)
{
	return awoke_tcp_do_connect(&c->_conn);
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

err_type http_request(const char *uri, const char *method, const char *protocol, 
	const char *body, struct _http_header *headers, struct _http_connect *alive, 
	char *rsp)
{
	err_type ret;
	struct _http_request *req;

	if (!uri || !body)
		return et_param;

	req = mem_alloc_z(sizeof(struct _http_request));
	if (!req) {
		log_err("alloc request error");
		return et_nomem;
	}

	http_request_init(req);

	ret = http_connect_create(&req->conn, uri);
	if (ret != et_ok) {
		log_err("connect create error, ret %d", ret);
		goto release;
	}

	req->host = req->conn.host;
	req->method = mem_mk_ptr(method);
	if (!strncmp(HTTP_METHOD_POST_STR, req->method.p, req->method.len)) {
		req->body = mem_mk_ptr(body);
	}
	req->protocol = mem_mk_ptr(protocol);
	req->extend_headers = headers;
	mem_ptr_copy(&req->path, &req->conn.path);
	
	ret = http_request_package(req);
	if (ret != et_ok) {
		log_err("package post request error");
		goto release;
	}

	ret = http_do_connect(&req->conn);
	if (ret != et_ok) {
		log_err("do connect error, ret %d", ret);
		goto release;
	}

	ret = http_request_send(req);
	if (ret != et_ok) {
		log_err("do connect error, ret %d", ret);
		goto release;
	}

	ret = http_request_recv(req);
	if (ret != et_ok) {
		log_err("do connect error, ret %d", ret);
		goto release;
	}

	http_state_code(req);

	if ((req->original_len > 0) && (rsp != NULL)) {
		memcpy(rsp, &req->original_buf, req->original_len);
	}

	if (req->state_code != 	HTTP_CODE_SUCCESS) {
		log_debug("state code error, code:%d", req->state_code);
	}
	
release:
	if (req)
		http_request_clean(&req);
	return ret;
}
