#include "server.h"

err_type message_process(rtmsg_handle *handle)
{
	rtmsg_header *header;
	err_type ret = et_ok;
	
	ret = awoke_rtmsg_recv_timeout(handle, &header, 3000);
	if (et_ok != ret) {
		log_err("rtmsg recv error, ret %d", ret);
		return et_ok;
	} 
		
	switch (header->msg_type) {
		case rtmsg_client_online:
			log_debug("client online");
			msg_client_online *data;
			data = (msg_client_online *)(header + 1);
			log_debug("client name %s, rid %d", data->name, data->rid);
			break;
	}
		
	if (header) mem_free(header);

	return et_ok;
}

int main(int argc, char **argv)
{
	rtmsg_handle *handle;
	err_type ret = et_ok;

	log_level(LOG_DBG);

	ret = awoke_rtmsg_init(app_test_server, &handle);
	if (et_ok != ret) {
		log_err("rtmsg init error, ret %d", ret);
		EXIT(AWOKE_EXIT_FAILURE);
	}

	while (1) {
		int num;
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(handle->fd, &rfds);

		num = select(handle->fd + 1, &rfds, NULL, NULL, 0);
		if (num > 0) {
			if (FD_ISSET(handle->fd, &rfds)) {
				message_process(handle);
			}
		}
	}

	awoke_rtmsg_handle_clean(&handle);
	return 0;
}
