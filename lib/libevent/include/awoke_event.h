#ifndef __AWOKE_EVENT_H__
#define __AWOKE_EVENT_H__

#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

#include "awoke_type.h"
#include "awoke_list.h"
#include "awoke_error.h"
#include "awoke_socket.h"


#define EVENT_QUEUE_SIZE        128


/* Events type family */
#define EVENT_NOTIFICATION      0    /* notification channel (pipe)      */
#define EVENT_LISTENER          1    /* listener socket                  */
#define EVENT_CONNECTION        2    /* data on active connection        */
#define EVENT_CUSTOM            3    /* custom fd registered             */

/* Event triggered for file descriptors  */
#define EVENT_EMPTY             0x0000
#define EVENT_READ              0x0001
#define EVENT_ERROR				0x0002
#define EVENT_WRITE             0x0004
#define EVENT_SLEEP             0x0008
#define EVENT_CLOSE             (0x0010 | 0x0008 | 0x2000)
#define EVENT_IDLE              (0x0010 | 0x0008)

/* Event status */
#define EVENT_NONE              1    /* nothing */
#define EVENT_REGISTERED        2    /* event is registered into the ev loop */


#define EP_SOCKET_CLOSED   0
#define EP_SOCKET_ERROR    1
#define EP_SOCKET_TIMEOUT  2
#define EP_SOCKET_DONE     3


/* Event reported by the event loop */
typedef struct _awoke_event {
    int      fd;       /* monitored file descriptor */
    int      type;     /* event type  */
    uint32_t mask;     /* events mask */
    uint8_t  status;   /* internal status */
    void    *data;     /* custom data reference */

    /* function handler for custom type */
    int     (*handler)(void *data);
    awoke_list _head;
} awoke_event;

typedef struct _awoke_event_pipech {
	awoke_event event;
	int ch_r;
	int ch_w;
} awoke_event_pipech;

typedef struct _awoke_event_ctx {
    int max_fd;

    /* Original set of file descriptors */
    fd_set rfds;
    fd_set wfds;
	fd_set efds;

    /* Populated before every select(2) */
    fd_set _rfds;
    fd_set _wfds;
	fd_set _efds;

    int queue_size;
    awoke_event **events;  /* full array to register all events */
    awoke_event *fired;    /* used to create iteration array    */
} awoke_event_ctx;


typedef struct _awoke_event_loop {
    int size;                   /* size of events array */
    int timeout;
    int n_events;               /* number of events reported */
    struct awoke_event *events;   /* copy or reference of events triggered */
    awoke_event_ctx *ctx;         /* mk_event_ctx_t from backend */
} awoke_event_loop;

#define awoke_event_foreach(event, evl)                                   \
    int __i;                                                            \
    awoke_event_ctx *__ctx = evl->ctx;                                    \
                                                                        \
    if (evl->n_events > 0) {                                            \
        event = __ctx->fired[0].data;                                   \
    }                                                                   \
                                                                        \
    for (__i = 0;                                                       \
         __i < evl->n_events;                                           \
         __i++,                                                         \
             event = __ctx->fired[__i].data                             \
         )

static inline void AWOKE_EVENT_NEW(awoke_event *e)
{
    e->mask   = EVENT_EMPTY;
    e->status = EVENT_NONE;
}


awoke_event_loop *awoke_event_loop_create(int size);
err_type awoke_event_add(awoke_event_loop *loop, int fd,
                 int type, uint32_t mask, void *data);
void awoke_event_del(awoke_event_loop *loop, awoke_event *event);
int awoke_event_wait(awoke_event_loop *loop, uint32_t tm);
err_type awoke_event_channel_create(awoke_event_loop *loop, 
									 int *r_fd, int *w_fd, void *data);
err_type awoke_event_pipech_create(awoke_event_loop *loop, awoke_event_pipech *ch);
void awoke_event_loop_clean(awoke_event_loop **loop);
void awoke_event_pipech_clean(awoke_event_loop *loop, awoke_event_pipech *ch);


#endif /* __WEB_EVENT_H__ */

