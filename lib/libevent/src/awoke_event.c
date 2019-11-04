#include "awoke_event.h"
#include "awoke_type.h"
#include "awoke_log.h"
#include "awoke_socket.h"

#include <sys/select.h>


void awoke_event_clean(awoke_event **event)
{
    awoke_event *pevent;
    
    if (!event || !*event)
        return;

    pevent = *event;

    if (pevent->fd)
        close(pevent->fd);

    mem_free(pevent);
    pevent = NULL;
    
    return;
}

static inline void awoke_event_loop_ctx_clean(awoke_event_ctx **ctx)
{
    awoke_event_ctx *pctx;
    
    if (!*ctx || !ctx)
        return;

    pctx = *ctx;

    if (&pctx->events)
        mem_free(&pctx->events);

    if (pctx->fired)
        mem_free(pctx->fired);
    
    mem_free(pctx);
    pctx = NULL;
    return;
}

void awoke_event_loop_clean(awoke_event_loop **loop)
{
    awoke_event_loop *ploop;

    if (!*loop || !loop)
        return;

    ploop = *loop;

    if (ploop->ctx)
        awoke_event_loop_ctx_clean(&ploop->ctx);

    if (ploop->events)
        mem_free(ploop->events);

    mem_free(ploop);
    ploop = NULL;
    return;
}

void awoke_event_del(awoke_event_loop *loop, awoke_event *event)
{
    int i;
    int fd;
    awoke_event *s_event;
    awoke_event_ctx *ctx = loop->ctx;
    
    /* just remove a registered event */
    if ((event->status & EVENT_REGISTERED) == 0) {
        return;
    }

    fd = event->fd;

    if (event->mask & EVENT_READ) {
        FD_CLR(event->fd, &ctx->rfds);
    }

    if (event->mask & EVENT_WRITE) {
        FD_CLR(event->fd, &ctx->wfds);
    }

     /* Update max_fd, lookup */
    if (event->fd == ctx->max_fd) {
        for (i = (ctx->max_fd - 1); i > 0; i--) {
            if (!ctx->events[i]) {
                continue;
            }

            s_event = ctx->events[i];
            if (s_event->mask != EVENT_EMPTY) {
                break;
            }
        }
        ctx->max_fd = i;
    }

    ctx->events[fd] = NULL;

    /* Reset the status and mask */
    AWOKE_EVENT_NEW(event);
        
    return;
}

err_type awoke_event_add(awoke_event_loop *loop, int fd,
                 int type, uint32_t mask, void *data)
{
    awoke_event *event;

	if (!loop) {
		return et_param;
	}

	awoke_event_ctx *ctx = loop->ctx;

    if (!loop || !data) {
        return et_param;
    }
        
    if (fd > FD_SETSIZE) {
        return et_param;
    }

    if (mask & EVENT_READ) {
        FD_SET(fd, &ctx->rfds);
    }
    
    if (mask & EVENT_WRITE) {
        FD_SET(fd, &ctx->wfds);
    }

    event = (awoke_event *) data;
    event->fd   = fd;
    event->type = type;
    event->mask = mask;
    event->status = EVENT_REGISTERED;

    ctx->events[fd] = event;
    if (fd > ctx->max_fd) {
        ctx->max_fd = fd;
    }

    return et_ok;
}

int awoke_event_wait(awoke_event_loop *loop, uint32_t tm)
{
    int i;
    int f = 0;
    uint32_t mask;
    awoke_event *fired;
    struct timeval tv;
    awoke_event_ctx *ctx = loop->ctx;

    memcpy(&ctx->_rfds, &ctx->rfds, sizeof(fd_set));
    memcpy(&ctx->_wfds, &ctx->wfds, sizeof(fd_set));

    memset(&tv, 0x0, sizeof(tv));
    tv.tv_sec = tm/1000;
    tv.tv_usec = tm%1000;

    loop->n_events = select(ctx->max_fd + 1, &ctx->_rfds, &ctx->_wfds, NULL, &tv);
    if (loop->n_events <= 0) {
        return loop->n_events;
    }

    for (i=0; i<= ctx->max_fd; i++)
    {
        /* skip empty references */
        if (!ctx->events[i]) {
            continue;
        }

        mask = 0;

        if (FD_ISSET(i, &ctx->_rfds)) {
            mask |= EVENT_READ;
        }

        if (FD_ISSET(i, &ctx->_wfds)) {
            mask |= EVENT_READ;
        }

        if (mask) {
            fired = &ctx->fired[f];
            fired->fd   = i;
            fired->mask = mask;
            fired->data = ctx->events[i];
            f++;
        }
    }

    loop->n_events = f;
    return loop->n_events;
}

bool awoke_event_ready(awoke_event *event, awoke_event_loop *evl)
{
    awoke_event_ctx *ctx = evl->ctx;       

    if (event->mask & EVENT_WRITE)
        return TRUE;
    
    if (event->mask & EVENT_READ) {
        if(FD_ISSET(event->fd, &ctx->_rfds)) {
            if (awoke_sock_fd_read_ready(event->fd)) {
                return TRUE;
            } else {
                awoke_event_del(evl, event);
            }
        }
    } 

    return FALSE;
}

inline bool awoke_event_read_ready(awoke_event *event)
{
    return mask_exst(event->mask, EVENT_READ);
}
inline bool awoke_event_write_ready(awoke_event *event)
{
    return mask_exst(event->mask, EVENT_WRITE);
}

static inline awoke_event_ctx *awoke_event_loop_ctx_create(int size)
{
    awoke_event_ctx *ctx;

    /* Override caller 'size', we always use FD_SETSIZE */
    size = FD_SETSIZE;

    /* Main event context */
    ctx = mem_alloc_z(sizeof(awoke_event_ctx));
    if (!ctx) {
        return NULL;
    }

    FD_ZERO(&ctx->rfds);
    FD_ZERO(&ctx->wfds);

    /* Allocate space for events queue, re-use the struct mk_event */
    ctx->events = mem_alloc_z(sizeof(awoke_event *) * size);
    if (!ctx->events) {
        mem_free(ctx);
        return NULL;
    }

    /* Fired events (upon select(2) return) */
    ctx->fired = mem_alloc_z(sizeof(awoke_event) * size);
    if (!ctx->fired) {
        mem_free(ctx->events);
        mem_free(ctx);
        return NULL;
    }

    ctx->queue_size = size;
    return ctx;
}


awoke_event_loop *awoke_event_loop_create(int size)
{
    awoke_event_ctx *ctx;
    awoke_event_loop *loop;

    ctx = awoke_event_loop_ctx_create(size);
    if (!ctx) {
		log_err("ctx create error");
        return NULL;
    }

    loop = mem_alloc_z(sizeof(awoke_event_loop));
    if (!loop) {
		log_err("loop create error");
        goto err;
    }

    loop->events = mem_alloc_z(sizeof(awoke_event) * size);
    if (!loop->events) {
		log_err("events create error");
        goto err;
    }

    loop->size   = size;
    loop->ctx   = ctx;
    return loop;


err:
    if (ctx)
        awoke_event_loop_ctx_clean(&ctx);
    
    if (loop)
        awoke_event_loop_clean(&loop);
    
    return NULL;
}

err_type awoke_event_channel_create(awoke_event_loop *loop, 
									 int *r_fd, int *w_fd, void *data)
{
	int rc;
	err_type ret;
	int fd[2];
	awoke_event *event;
	
	rc = pipe(fd);
	if (rc < 0) {
		return et_pipe;
	}

	event = data;
	event->fd = fd[0];
	event->type = EVENT_NOTIFICATION;
	event->mask = EVENT_EMPTY;

	ret = awoke_event_add(loop, fd[0], 
						  EVENT_NOTIFICATION, EVENT_READ, event);
	if (ret != et_ok) {
		close(fd[0]);
		close(fd[1]);
		return ret;
	}

	event->mask = EVENT_READ;
	
	*r_fd = fd[0];
	*w_fd = fd[1];

	return et_ok;
}

err_type awoke_event_pipech_create(awoke_event_loop *loop, awoke_event_pipech *ch)
{
	return awoke_event_channel_create(loop, &ch->ch_r, &ch->ch_w, &ch->event);
}

void awoke_event_pipech_clean(awoke_event_loop *loop, awoke_event_pipech *ch)
{
	awoke_event_del(loop, &ch->event);
	close(ch->ch_r);
	close(ch->ch_w);
}