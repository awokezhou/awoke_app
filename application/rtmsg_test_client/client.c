#include "client.h"


err_type client_msg_send(rtmsg_handle *handle)
{
	err_type ret = et_ok;
	rtmsg_header *header;
	msg_client_online *data;
	
	char msg_buff[sizeof(rtmsg_header) + sizeof(msg_client_online)];
	data = (msg_client_online *)(msg_buff + sizeof(rtmsg_header));

	sprintf(data->name, "%s", "test_client");
	data->rid = app_test_client;

	header = (rtmsg_header *)msg_buff;
	header->dst = app_test_server;
	header->src = app_test_client;
	header->msg_type = rtmsg_client_online;
	header->data_len = sizeof(msg_client_online);

	ret = awoke_rtmsg_send(handle, header);
	if (et_ok != ret) {
		log_err("rtmsg send message error, ret %d", ret);
	}

	return ret;
}

int main(int argc, char **argv)
{
	rtmsg_handle *handle;
	err_type ret = et_ok;

	log_level(LOG_DBG);
	
	ret = awoke_rtmsg_init(app_test_client, &handle);
	if (et_ok != ret) {
		log_err("rtmsg init error, ret %d", ret);
		EXIT(AWOKE_EXIT_FAILURE);
	}

	while (1) {
		client_msg_send(handle);
		log_debug("msg send");
		sleep(1);
	}

	awoke_rtmsg_handle_clean(&handle);
	return 0;
}
