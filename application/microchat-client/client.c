
#include "client.h"
#include "awoke_log.h"
#include "awoke_cmdline.h"
#include "reactor.h"
#include "server.h"


err_type action_exit(struct _cmdline_item *item);
err_type action_record(struct _cmdline_item *item);
err_type action_textchat(struct _cmdline_item *item);
err_type action_handshark(struct _cmdline_item *item);
err_type action_heartbeat(struct _cmdline_item *item);
err_type action_voicechat(struct _cmdline_item *item);



static awoke_cmdline_item command_table[] = {
	{"exit", 		"Exit", 						"exit", 		action_exit},
	{"record", 		"Recording environment sound", 	"record", 		action_record},
	{"textchat", 	"Send a piece of text", 		"textchat", 	action_textchat},
	{"handshark", 	"Do handshark with server", 	"handshark", 	action_handshark},
	{"heartbeat", 	"Send heartbeat to server", 	"heartbeat", 	action_heartbeat},
	{"voicechat", 	"Send a piece of audio", 		"voicechat", 	action_voicechat},
};

static struct _awoke_worker *reactor;

err_type action_handshark(struct _cmdline_item *item)
{
	log_info("Action:%s", item->name);
	handshark_msg_send();
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

void destroy_message(void *_msg)
{
	struct msg *msg = _msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
	log_trace("message destory");
}

int main(int argc, char **argv)
{
	err_type ret;
	struct _awoke_worker *wk;

	log_info("micro-chat client start");
	
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

	return 0;
}
