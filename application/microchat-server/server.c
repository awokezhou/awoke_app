
#include "server.h"
#include "reactor.h"
#include "awoke_log.h"
#include "awoke_worker.h"
#include "awoke_cmdline.h"


err_type action_exit(struct _cmdline_item *item);
err_type action_record(struct _cmdline_item *item);
err_type action_textchat(struct _cmdline_item *item);
err_type action_handshark(struct _cmdline_item *item);
err_type action_heartbeat(struct _cmdline_item *item);
err_type action_voicechat(struct _cmdline_item *item);



struct _awoke_worker *reactor;

static awoke_cmdline_item command_table[] = {
	{"exit", 		"Exit", 						"exit", 		action_exit},
	{"record", 		"Recording environment sound", 	"record", 		action_record},
	{"textchat", 	"Send a piece of text", 		"textchat", 	action_textchat},
	{"handshark", 	"Do handshark with server", 	"handshark", 	action_handshark},
	{"heartbeat", 	"Send heartbeat to server", 	"heartbeat", 	action_heartbeat},
	{"voicechat", 	"Send a piece of audio", 		"voicechat", 	action_voicechat},
};

err_type action_handshark(struct _cmdline_item *item)
{
	log_info("Action:%s", item->name);
	return et_ok;
}

err_type action_record(struct _cmdline_item *item)
{
	log_info("Action:%s", item->name);
	return et_ok;
}

err_type action_heartbeat(struct _cmdline_item *item)
{
	log_info("Action:%s", item->name);
	return et_ok;	
}

err_type action_voicechat(struct _cmdline_item *item)
{
	log_info("Action:%s", item->name);
	return et_ok;	
}

err_type action_textchat(struct _cmdline_item *item)
{
	log_info("Action:%s", item->name);
	return et_ok;
}

err_type action_exit(struct _cmdline_item *item)
{
	log_info("Action:%s", item->name);
	awoke_worker_stop(reactor);
	return et_exist;		
}

err_type server_handshark_process()
{
	return handshark_msg_send();
}

int main(int argc, char **argv)
{
	err_type ret;
	struct _awoke_worker *wk;

	wk = awoke_worker_create("Reactor", 0, WORKER_FEAT_CUSTOM_DEFINE,
						 	 reactor_worker, NULL);

	reactor = wk;

	do {
		ret = awoke_cmdline_wait(command_table, array_size(command_table));
	} while ((ret == et_ok) && 
			 (!awoke_worker_should_stop(reactor)));	

	awoke_worker_destroy(reactor);

	if (ret == et_exist) {
		log_debug("Exit");
	} else {
		log_err("Error:%d", ret);
	}	
}
