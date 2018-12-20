#include "router_main.h"
#include "router_forward.h"


void forward_item_free(forward_item **item)
{
	forward_item *p;

	if (!item || !*item)
		return;

	p = *item;

	if (p->fd)
		close(p->fd);
	mem_free(p);
	p = NULL;
	return;
}

void router_fdt_clean(struct _rt_manager *manager)
{
	forward_item *temp;
	forward_item *item;
	awoke_list *fdt = &manager->forward_list;

	list_for_each_entry_safe(item, temp, fdt, _head) {
		list_unlink(&item->_head);
		forward_item_free(&item);
	}
}

void forward_queue_message(forward_item *dst, rtmsg_header **header)
{
	(*header)->next = dst->queue;
	dst->queue = (*header);
	*header = NULL;
}

err_type forward_deliver_broadcast(rtmsg_header **header, awoke_list *fdt)
{
	forward_item *dst;

	list_for_each_entry(dst, fdt, _head) {
		rt_message_send(dst->fd, *header);
	}

	mem_free(*header);
	return et_ok;
}

err_type forward_deliver_unicast(rtmsg_header **header, awoke_list *fdt)
{
	forward_item *dst;
	err_type ret = et_ok;

	dst = forward_item_get_byrid((*header)->dst, fdt);
	if (!dst) {
		return et_exist;
	}

	if (dst->status == app_not_run) {
		forward_queue_message(dst, *header);
	} else if (dst->status == app_launched) {
		forward_queue_message(dst, *header);
	} else if (dst->status == app_running) {
		ret = rt_message_send(dst->fd, *header);
		mem_free(*header);
	} else if (dst->status == app_termination_request) {
		mem_free(*header);
	}

	return et_ok;
}

err_type router_fdt_deliver(rtmsg_header **header, rt_manager *manager)
{
	awoke_list *fdt = &manager->forward_list;

	if ((*header)->f_broadcast) {
		return forward_deliver_broadcast(header, fdt);
	} else {
		return forward_deliver_unicast(header, fdt);
	}
}

err_type router_fdt_app_associate(int fd, rt_manager *manager)
{
	forward_item *item;
	err_type ret = et_ok;
	rtmsg_header *header;
	uint32_t tm = RT_TM_ASSOCIATE_MSG_RECV;
	awoke_list *fdt = &manager->forward_list;
	
	ret = rt_message_recv(fd, &header, &tm);
	if (et_ok != ret) {
		log_err("rt message receive error, ret %d", ret);
		return ret;
	}

	if ((header->msg_type != rtmsg_associate) || 
		(header->dst != app_message_router)) {
		log_err("illegal mesager");
		return et_param;
	}

	if ((header->src <= app_none) ||
		(header->src >= app_max)) {
		log_err("error src");
		return et_param;
	}

	item = forward_item_get_byrid(header->src, fdt);
	if (!item) {
		log_err("item not find");
		return et_exist;
	}

	item->fd = fd;
	item->status = app_running;
	log_debug("rtmsg: %s associated, fd %d", item->rt_info->name, item->fd);
	mem_free(header);
	return et_ok;
}

forward_item *forward_item_get_byrid(rt_app_id rid, awoke_list *fdt)
{
	forward_item *item;

	list_for_each_entry(item, fdt, _head) {
		if (item->rt_info->rid == rid)
			return item;
	}

	return NULL;
}

err_type forward_item_register(rtmsg_app_info *info, awoke_list *fdt)
{
	forward_item *temp;
	forward_item *item;

	temp->rt_info->name;

	list_for_each_entry(temp, fdt, _head) {
		rtmsg_app_info *tif = temp->rt_info;
		if (!strcmp(tif->name, info->name) &&
			tif->rid == info->rid) {
			return et_exist;	
		}
	}

	item = mem_alloc_z(sizeof(forward_item));
	if (!item) 
		return et_nomem;

	item->rt_info = info;
	item->status = app_not_run;
	list_prepend(&item->_head, fdt);
	return et_ok;
}

void router_fdt_debug(struct _rt_manager *manager)
{
	forward_item *item;
	awoke_list *fdt = &manager->forward_list;

	list_for_each_entry(item, fdt, _head) {
		rtmsg_app_info *info = item->rt_info;
		log_debug("item name %s, rid %d, stat %d", 
			info->name, info->rid, item->status);
	}
}

err_type router_fdt_init(struct _rt_manager *manager)
{
	int size;
	rtmsg_app_info *head;
	rtmsg_app_info *info;
	awoke_list *fdt = &manager->forward_list;

	head = get_app_array();
	size = get_app_array_size();

	if (!head || size <= 0)
		return et_fail;

	rt_app_foreach(head, size, info) {
		forward_item_register(info, fdt);
	}

	//router_fdt_debug(manager);

	return et_ok;
}
