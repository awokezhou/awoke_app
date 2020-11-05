
#include "router_main.h"
#include <getopt.h>


static void router_help(int ex)
{
	printf("usage : message-router [option]\n\n");

    printf("    -d\t--daemon\t\trun fdmn_drv_bhv as daemon\n");
    printf("    -l\t--loglevel\t\tset log level, 1:debug 2:info 3:warning 4:error 5:bug\n");
	printf("    -l\t--logmode\t\tset log mode, 1:test  2:system\n");

	EXIT(ex);
}


void router_param_clean(rt_param **param)
{
    rt_param *p;

    if (!*param || !param)
        return;

    p = *param;

    mem_free(p);
    p = NULL;
    return;
}

void router_manager_clean(rt_manager **manager)
{
    rt_manager *p;
	rt_param *param; 

    if (!*manager || !manager)
        return;

    p = *manager;
	param = p->param;

	router_fdt_clean(p);
	router_param_clean(&param);

   	mem_free(p);
    p = NULL;
    return;    
}

rt_param *router_param_create()
{	
	rt_param *param = NULL;

	param = mem_alloc_z(sizeof(rt_param));
	if (!param)
		return NULL;

	param->daemon = FALSE;
	param->log_mode = LOG_M_SYS;
	param->log_level = LOG_DBG;

	return param;
}

rt_param * router_param_parse(int argc, char *argvs[])
{
	int opt;
    rt_param *param = NULL;

	param = router_param_create();
    if (!param)
    {
       	log_err("param create error");
        return NULL;
    }

	static const struct option long_opts[] = {
        {"daemon",      no_argument,        NULL,   'd'},
        {"loglevel",   	required_argument,  NULL,   'l'},
        {"logmode",		required_argument, 	NULL,   'm'},
        {""},
        {NULL, 0, NULL, 0}
    };

	while ((opt = getopt_long(argc, argvs, "da:l:m:?h", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'd':
                param->daemon = TRUE;
                break;
                
            case 'l':
                param->log_level = atoi(optarg);
                break;

			case 'm':
				param->log_mode = atoi(optarg);
				break;

            case '?':
            case 'h':
            default:
                router_help(AWOKE_EXIT_SUCCESS);
        }
    }

	return param;
}

rt_manager *router_manager_create(rt_param *param)
{
	rt_manager *manager;

	if (!param)
        return NULL;

	manager = mem_alloc_z(sizeof(rt_manager));
    if (!manager)
        return NULL;

	manager->param = param;

	list_init(&manager->conn_list);
	list_init(&manager->forward_list);
	list_init(&manager->listener_list);

	return manager;
}

void router_run_daemon(rt_manager *manager)
{
    pid_t pid;

	if (!manager->param->daemon)
		return;

    pid = fork();

    switch (pid) {
        case -1:                        
            log_err("fork error!");
            return;

        case 0:                         // child
            break;

        default:                        // father
            EXIT(0);                
    }

    pid = setsid();
    if (pid < 0) {
        log_err("setsid error");
        return;
    }

	log_debug("run daemon ok");
    return;
}

static err_type listener_register(const char *name, uint8_t type, 
										 const char *address, uint16_t port,
										 rt_manager *manager)
{
	int fd;
	awoke_event *event;
	router_listener *listener;
	router_listener *new_listener;

	list_for_each_entry(listener, &manager->listener_list, _head) {
		if (!strcmp(listener->name, name))
			return et_exist;
	}

	fd = awoke_socket_server(type, address, 0, FALSE);
	if (fd < 0) {
		return et_sock_creat;
	}

	new_listener = mem_alloc_z(sizeof(router_listener));
	if (!new_listener)
		return et_nomem;
	
	new_listener->fd = fd;
	new_listener->port = port;
	new_listener->type = type;
	new_listener->name = awoke_string_dup(name);
	new_listener->address = awoke_string_dup(address);

	event = &new_listener->event;
	event->fd = fd;
	event->type = EVENT_LISTENER;
	event->mask = EVENT_EMPTY;
	event->status = EVENT_NONE;
	
	list_append(&new_listener->_head, &manager->listener_list);
	return et_ok;
}


err_type router_listener_init(rt_manager *manager)
{
	listener_register("[listener-ipc]", SOCK_LOCAL, RTMSG_SOCK_ADDR, 0, manager);
	return et_ok;
}

err_type router_init(rt_manager *manager)
{
	err_type ret = et_ok;
	rt_param *param = manager->param;

	router_signal_init(manager);
	log_debug("signal init ok");

	router_run_daemon(manager);

	ret = router_listener_init(manager);
	if (et_ok != ret) {
		log_err("listener init error, ret %d", ret);
		return ret;
	}
	log_debug("listener init ok");

	ret = router_fdt_init(manager);
	if (et_ok != ret) {
		log_err("forward table init error, ret %d", ret);
		return ret;
	}
	log_debug("forward table init ok");

	return ret;
}

void router_conn_free(router_conn **conn)
{
	router_conn *p;

	if (!conn || !*conn)
		return;

	p = *conn;

	mem_free(p);
	p = NULL;
	return;
}

router_conn *router_conn_add(int fd, router_listener *listener,
							       rt_manager *manager)
{
	router_conn *conn;
	awoke_event *event;

	conn = mem_alloc_z(sizeof(router_conn));
	if (!conn)
		return NULL;

	event = &conn->event;
	event->fd = fd;
    event->type = EVENT_CONNECTION;
    event->mask = EVENT_EMPTY;
    event->status = EVENT_NONE;    

	conn->status = CONN_OPEN;
	conn->listener = listener;
	list_prepend(&conn->_head, &manager->conn_list);
	return conn;
}

err_type router_message_proc(rtmsg_header **header, rt_manager *manager)
{
	awoke_list *fdt = &manager->forward_list;

	if ((*header)->f_broadcast) {
		return forward_deliver_broadcast(header, fdt);
	}

	if ((*header)->f_event) {
		//return router_message_event_proc();
	} else {
		//return router_message_request_proc();
	}
}

err_type router_connect_proc(awoke_event_loop *evl, 
									 awoke_event *event,
									 rt_manager *manager)
{
	rtmsg_header *header;
	err_type ret = et_ok;

	ret = rt_message_recv(event->fd, &header);
	if (et_ok != ret) {
		log_err("rt messager receive error, ret %d", ret);
		return ret;
	}

	if (header->dst == app_message_router) {
		ret = router_message_proc(&header, manager);
	} else {
		ret = router_fdt_deliver(&header, manager);
	}

	return et_ok;
}

err_type router_listener_proc(awoke_event_loop *evl, 
									  awoke_event *event,
									  rt_manager *manager)
{
	router_conn *conn;
	int client_fd = -1;
	int server_fd = event->fd;
	router_listener *listener = event;
	err_type ret = et_ok;
	
	client_fd = awoke_socket_accept_unix(server_fd);
	if (client_fd < 0) {
		log_err("socket accpet error");
		return et_sock_accept;
	}

	if (listener->type == SOCK_LOCAL)
		ret = router_fdt_app_associate(client_fd, manager);
	else {
		log_err("not support");
		goto err;
	}

	if (et_ok != ret) {
		goto err;
	}

	conn = router_conn_add(client_fd, listener, manager);
	if (!conn) {
		log_err("conn mem error");
		ret = et_nomem;
		goto err;
	}
	
	ret = awoke_event_add(evl, client_fd, EVENT_CONNECTION, EVENT_READ, conn);
	if (awoke_unlikely(et_ok != ret)) {
		log_err("event add error, ret %d", ret);
		goto err;
	}

	return et_ok;

err:
	if (client_fd)
		close(client_fd);
	if (conn)
		router_conn_free(&conn);
	return ret;
}

err_type router_main_loop(rt_manager *manager)
{
	awoke_event *event;
	awoke_event_loop *evl;
	router_listener *listener;
	err_type ret = et_ok;

	evl = awoke_event_loop_create(EVENT_QUEUE_SIZE);
    if (!evl)
    {
        log_err("event loop create error");
        return et_evl_create;
    }

	/* Register the listeners */
	list_for_each_entry(listener, &manager->listener_list, _head) {
        awoke_event_add(evl, 
			            listener->fd, 
                        EVENT_LISTENER, 
                        EVENT_READ,
                        listener);
    }

	while (1) {
		
		awoke_event_wait(evl, 1000);
		awoke_event_foreach(event, evl) {
		
			if (!awoke_event_ready(event, evl))
                continue;

			if (event->type == EVENT_LISTENER) {
				log_debug("EVENT_LISTENER");
				router_listener_proc(evl, event, manager);
			} else if (event->type == EVENT_CONNECTION) {
				log_debug("EVENT_CONNECTION");
				router_connect_proc(evl, event, manager);
			} else if (event->type == EVENT_NOTIFICATION) {
				/**/
			} else if (event->type == EVENT_CUSTOM) {
				/**/
			}
		}
	}
	
	return et_ok;
}

int main(int argc, char **argv)
{
	err_type ret = et_ok;
	rt_param *param;
	rt_manager *manager;

	log_info("application message-router launched...");

	param = router_param_parse(argc, argv);
	if (!param) {
		log_err("param parse error, will exit...");
		EXIT(AWOKE_EXIT_FAILURE);
	}

	manager = router_manager_create(param);
	if (!manager) {
		log_err("manager create error, will exit...");
		router_param_clean(&param);
		EXIT(AWOKE_EXIT_FAILURE);
	}

	ret = router_init(manager);
	if (et_ok != ret) {
		log_err("router init error, will exit...");
		goto err;
	}

	ret = router_main_loop(manager);
	log_info("main loop return, ret %d", ret);

err:
	router_manager_clean(&manager);
	EXIT(AWOKE_EXIT_SUCCESS);
}
