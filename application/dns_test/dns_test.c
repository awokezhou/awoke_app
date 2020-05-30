
#include "dns_test.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


static dns_server_entry g_dns_server_fix_table[] = {
	//{"8.8.8.8"},
	{"114.114.114.114"},
	{"172.16.79.1"},
};
static int g_dns_server_fix_table_size = array_size(g_dns_server_fix_table);

static dns_entry g_dns_fix_table[] = {
	{"www.github.com", {192,168,1,132}},
};
static int g_dns_fix_table_size = array_size(g_dns_fix_table);

static awoke_list g_dns_table;
static awoke_list g_dns_server_list;

void dns_request_hostname_add(dns_request *req, const char *hostname)
{
	if (req->host_nr >= DNS_RR_NR) 
		return et_param;

	req->host[req->host_nr] = mem_mk_ptr(hostname);
	req->host_nr++;
}

err_type dns_request_package_header(dns_request *req)
{	
	static int transaction_id = 0;

	transaction_id++;
	
	req->message.header.transaction_id = transaction_id;
	req->message.header.flags = htons(DNS_F_QUERY);
	req->message.header.questions = htons(DNS_ONE_QUESTION);
	req->message.header.answers_RRs = 0;
	req->message.header.authority_RRs = 0;
	req->message.header.additional_RRs = 0;

	return et_ok;
}

void dns_request_package_question(dns_request *req)
{
	int i;
	int size = req->host_nr;

	for (i=0; i<size; i++) {
		mem_ptr_t host = req->host[i];
		dns_rr *question = &req->message.question[i];
		strncpy(&question->name, host.p, host.len);
		question->class = htons(DNS_CLASS);
		question->type = htons(DNS_T_A);
	}
}

err_type dns_request_package(dns_request *req)
{
	int i;
	int rc;
	uint8_t nlen;
	uint8_t zero = 0x0;
	uint8_t *pos = req->original_buf;
	uint8_t *end = pos + DNS_REQUEST_LEN;
	uint8_t *p;
	
	req->us_class = htons(DNS_CLASS);
	req->us_record_type = htons(DNS_TYPE_IPV4);

	dns_request_package_header(req);
	dns_request_package_question(req);
	
	/* header push */
	pkg_push_word_safe(req->message.header.transaction_id, pos, end);
	pkg_push_word_safe(req->message.header.flags, pos, end);
	pkg_push_word_safe(req->message.header.questions, pos, end);
	pkg_push_word_safe(req->message.header.answers_RRs, pos, end);
	pkg_push_word_safe(req->message.header.authority_RRs, pos, end);
	pkg_push_word_safe(req->message.header.additional_RRs, pos, end);

	/* question push */
	for (i=0; i<req->host_nr; i++) {
		dns_rr *question = &req->message.question[i];
		uint8_t *name = question->name;
		while (p = strchr(name, '.')) {
			nlen = p - name;
			pkg_push_byte_safe(nlen, pos, end);
			pkg_push_stris_safe(name, pos, end, nlen);
			name += nlen+1;
		}
		p = strchr(name, 0x0);
		nlen = p - name;
		pkg_push_byte_safe(nlen, pos, end);
		pkg_push_stris_safe(name, pos, end, nlen);
		pkg_push_byte_safe(zero, pos, end);
	}
	
	pkg_push_word_safe(req->us_record_type, pos, end);
	pkg_push_word_safe(req->us_class, pos, end);

	req->original_len = pos - req->original_buf;

	return et_ok;
}

err_type dns_request_send(dns_request *req, int fd)
{
	int rc;

	rc = send(fd, req->original_buf, req->original_len, 0);
	if ((rc < 0) || (rc != req->original_len)) {
		log_err("send error");
		return et_sock_send;
	}

	return et_ok;
}

err_type dns_request_recv(dns_request *req, int fd)
{
	int rc;

	rc = read(fd, req->original_buf, DNS_REQUEST_LEN);
	if (rc < 0) {
		log_err("recv error");
		return et_sock_recv;
	}

	return et_ok;
}

static uint8_t *skip_name_field(uint8_t *p)
{
	if((*p & DNS_NAME_OFFSET) == DNS_NAME_OFFSET) {
		p += 2;
	} else {
		while(*p != 0x00) {
			p += (*p + 1);
		}
		p++;
	}
	return p;
}

err_type dns_table_update(dns_request *req)
{
	int i;
	dns_entry *e;
	bool find = FALSE;

	dns_rr *answer = &req->message.answer[req->addr_valid];
	
	list_for_each_entry(e, &g_dns_table, _head) {
		if (!strncmp(req->host[0].p, e->hostname, req->host[0].len)) {
			find = TRUE;
			uint32_t _addr1, _addr2;
			memcpy(&_addr1, answer->data, answer->rdatalen);
			memcpy(&_addr2, e->ipaddr, 4);
			if (_addr1 == _addr2) {
				return et_ok;
			} else {
				memcpy(e->ipaddr, &_addr1, 4);
				return et_ok;
			}
		}
	}

	if (!find) {
		log_debug("not find");
		dns_entry *new = mem_alloc_z(sizeof(dns_entry));
		memcpy(new->ipaddr, answer->data, answer->rdatalen);
		memcpy(new->hostname, req->host[0].p, req->host[0].len);
		list_prepend(&new->_head, &g_dns_table);
	}

	return et_ok;
}

err_type dns_parse_reply(dns_request *req)
{
	int i;
	uint8_t *head = req->original_buf;
	uint8_t *pos = head;

	const uint16_t us_record_type_ipv4 = htons(DNS_TYPE_IPV4);
	const uint16_t us_record_type_ipv6 = htons(DNS_TYPE_IPV6);
	uint32_t ttl;
	struct sockaddr_in addr;

	dns_header *header = &req->message.header;
	
	pkg_pull_word(header->transaction_id, pos);
	pkg_pull_word(header->flags, pos);
	pkg_pull_word(header->questions, pos);
	pkg_pull_word(header->answers_RRs, pos);
	pkg_pull_word(header->authority_RRs, pos);
	pkg_pull_word(header->additional_RRs, pos);

	header->transaction_id = htons(header->transaction_id);
	header->flags = htons(header->flags);
	header->questions = htons(header->questions);
	header->answers_RRs = htons(header->answers_RRs);
	header->authority_RRs = htons(header->authority_RRs);
	header->additional_RRs = htons(header->additional_RRs);

	for (i=0; i<header->questions; i++) {
		dns_rr *question = &req->message.question[i];
		pos = skip_name_field(pos);
		pos += 4;
	}

	for(i=0; i<header->answers_RRs; i++) {
		dns_rr *answer = &req->message.answer[i];
		pos = skip_name_field(pos);
		pkg_pull_word(answer->type, pos);
		pkg_pull_word(answer->class, pos);
		pkg_pull_dwrd(answer->ttl, pos);
		pkg_pull_word(answer->rdatalen, pos);
		answer->type = htons(answer->type);
		answer->class = htons(answer->class);
		answer->ttl = htonl(answer->ttl);
		answer->rdatalen = htons(answer->rdatalen);
		if (answer->type == DNS_TYPE_IPV4) {
			if (answer->rdatalen != 4) {
				pos += answer->rdatalen;
				continue;
			}
			pkg_pull_bytes(answer->data, pos, answer->rdatalen);
			if (req->addr_valid == -1) 
				req->addr_valid = i;
			memcpy(&addr.sin_addr.s_addr, answer->data, answer->rdatalen);
			log_info("address %s", inet_ntoa(addr.sin_addr));
		} else {
			pos += answer->rdatalen;
		}
	}

	return et_ok;
}

err_type dns_server_request(const char *hostname, char *server_addr, 
	struct sockaddr_in *r_addr)
{
	int fd;
	int rc;
	err_type ret;
	dns_request req;
	struct sockaddr_in _server_addr;
	int server_len = sizeof(struct sockaddr_in);

	fd = awoke_socket_create(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("socket creat error");
		ret = et_sock_creat;
		goto ret;
	}

	log_debug("socket create ok");

	log_debug("server addr %s", server_addr);
	memset(&_server_addr, 0x0, sizeof(struct sockaddr_in));
	_server_addr.sin_family = AF_INET;
    _server_addr.sin_port = htons(53);
    _server_addr.sin_addr.s_addr = inet_addr(server_addr);

	log_debug("socket connect ok");

	memset(&req, 0x0, sizeof(dns_request));
	req.addr_valid = -1;
	dns_request_hostname_add(&req, hostname);
	dns_request_package(&req);

	rc = sendto(fd, req.original_buf, req.original_len, 0, 
			    (struct sockaddr *)&_server_addr, server_len);
	if (rc < 0) {
		log_err("send error, rc %d", rc);
		ret = et_sock_send;
		goto ret;
	}

	log_debug("send ok");

	memset(req.original_buf, 0x0, DNS_REQUEST_LEN);
	rc = recvfrom(fd, &req.original_buf, 
				  DNS_REQUEST_LEN, 0, (struct sockaddr *)&_server_addr, &server_len);
	if (rc < 0) {
		log_err("recv error, rc %d", rc);
		ret = et_sock_recv;
		goto ret;
	}

	log_debug("recv len %d", rc);

	ret = dns_parse_reply(&req);
	if (ret != et_ok) {
		log_err("parse reply error, ret %d", ret);
		ret = et_sock_listen;
	}

	if (req.message.header.answers_RRs > 0)
		ret = dns_table_update(&req);
	
ret:
	if (fd) close(fd);
	return ret;
}

err_type dns_table_lookup(const char *hostname, uint8_t ipaddr[])
{
	dns_entry *e;
	
	if (list_empty(&g_dns_table))
		list_init(&g_dns_table);

	list_for_each_entry(e, &g_dns_table, _head) {
		if (!strcmp(e->hostname, hostname)) {
			log_debug("find hostname");
			memcpy(ipaddr, e->ipaddr, 4);
			return et_ok;
		}
	}

	return et_exist;
} 

err_type dns_resolve(const char *hostname, struct sockaddr_in *addr, int timeout)
{	
	err_type ret;
	dns_server_entry *se;
	ret = dns_table_lookup(hostname, addr);
	if (ret == et_ok) {
		log_debug("find in dns table");
		return ret;
	}

	list_for_each_entry(se, &g_dns_server_list, _head) {
		ret = dns_server_request(hostname, se->addr, addr);
		if (ret == et_ok) {
			return ret;
		}
	}
	
	return et_exist;
}

err_type dns_server_list_init()
{
	int size;
	dns_server_entry *p;
	dns_server_entry *se;
	dns_server_entry *head;
	
	list_init(&g_dns_server_list);

	head = g_dns_server_fix_table;
	size = g_dns_server_fix_table_size;

	array_foreach(head, size, p) {
		log_debug("add server %s to server list", p->addr);
		list_prepend(&p->_head, &g_dns_server_list);
	}

	return et_ok;
}

err_type dns_table_init()
{
	int size;
	dns_entry *e;
	dns_entry *p;
	dns_entry *head;
	
	list_init(&g_dns_table);

	head = g_dns_fix_table;
	size = g_dns_fix_table_size;	

	array_foreach(head, size, p) {
		log_debug("add entry %s:"print_ip_format" to dns table", p->hostname, 
			print_ip(p->ipaddr));
		list_prepend(&p->_head, &g_dns_table);
	}	
}

err_type dns_init(void)
{
	int i;
	dns_entry *de;
	dns_entry *def;
	dns_server_entry *se;

	dns_table_init();
	dns_server_list_init();

	list_for_each_entry(de, &g_dns_table, _head) {
		log_debug("dns entry %s:"print_ip_format, de->hostname, print_ip(de->ipaddr));
	}

	list_for_each_entry(se, &g_dns_server_list, _head) {
		log_debug("dns server %s", se->addr);
	}

	list_for_each_entry_safe(de, def, &g_dns_table, _head) {
		list_unlink(&de->_head);
		break;
	}

	list_for_each_entry(de, &g_dns_table, _head) {
		log_debug("dns entry %s:"print_ip_format, de->hostname, print_ip(de->ipaddr));
	}

	return et_ok;
}

int main(int argc, char **argv)
{
	log_mode(LOG_TEST);
	log_level(LOG_DBG);

	dns_init();

	log_debug("dns init ok");

	struct sockaddr_in addr;
	err_type ret = dns_resolve("unicore-api1.rx-networks.cn", &addr, 5);
	log_debug("address %s", inet_ntoa(addr.sin_addr));

	dns_entry *e;
	list_for_each_entry(e, &g_dns_table, _head) {
		log_debug("dns entry %s:"print_ip_format, e->hostname, print_ip(e->ipaddr));
	}
	
	return 0;
}