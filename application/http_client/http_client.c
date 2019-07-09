
#include <getopt.h>

#include "mongoose.h"
#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"

typedef enum {
	ARG_NONE = 0,
	ARG_SHOW_HEADERS,
	ARG_SHOW_BODYS,
} arg_opt;

static bool g_http_poll = true;
static bool g_show_headers = false;
static bool g_show_bodys = false;
static char *g_url = "www.baidu.com";
static char *g_method = "GET";

void usage(int rc)
{
	printf("usage : http_client [option]\n\n");

    printf("    -u,\t\t--url\t\t\thttp url\n");
    printf("    -m,\t\t--method\t\thttp method, GET/POST\n");
	printf("    \t\t--show_headers\t\tshow http headers\n");

	EXIT(rc);	
}

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "awoke_memory.h"
void getaddr_test()
{
	log_debug("getaddr_test");

	int ret;
	int port = 80;
	unsigned long len;
	char *port_str = 0;
	struct addrinfo hints;
	struct addrinfo *res, *rp;
	char *host = "www.baidu.com";

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

	port_str = awoke_string_build(&port_str, &len, "%d", port);
	if (!port_str) {
		log_err("string build error");
		return;
	}
	log_debug("awoke_string_build ok");
	ret = getaddrinfo(host, port_str, &hints, &res);
	log_debug("getaddrinfo ok");
	mem_free(port_str);
	if(ret != 0) {
		log_err("Can't get addr info");
		return;
	}

	for (rp = res; rp != NULL; rp = rp->ai_next) {
		log_info("addr %d.%d.%d.%d", rp->ai_addr->sa_data[0],
			rp->ai_addr->sa_data[1],
			rp->ai_addr->sa_data[2],
			rp->ai_addr->sa_data[3]);
	}
	
	freeaddrinfo(res);
	return;
}

int main(int argc, char **argv)
{
	log_mode(LOG_TEST);
	log_level(LOG_DBG);

	/*
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

	mg_connect_http(&mgr, ev_handler, g_url, NULL, g_method);

	while (g_http_poll) {
		log_debug("polling");
    	mg_mgr_poll(&mgr, 1000);
  	}
  	mg_mgr_free(&mgr);

	log_debug("mgr free");
	*/
	
	getaddr_test();
	
	return 0;
}
