
#ifndef __CMDER_H__
#define __CMDER_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_list.h"
#include "awoke_log.h"



typedef enum {
    MATCH_STRING,
    MATCH_BYTE,
    MATCH_BYTES,
    MATCH_STRUCT,
} command_match_type;



struct cmder;
struct command_table;

struct command_match {
    uint8_t type;
    unsigned int length;
    union {
        const char *string;
        uint8_t byte;
        uint8_t *bytes;
        void *struc;
    } val;
};

struct command {
    struct command_match match;
    err_type (*get)(struct command *);
    err_type (*set)(struct command *);
    err_type (*exe)(struct command *);
};

struct command_matcher {
    bool (*format)(void *buf, int len);
    bool (*match)(struct command_table *tb, void *buf, int len, struct command **cmd);
};

struct command_table {

    const char *name;
    
    struct command_matcher *matcher;

    unsigned int nr_commands;
    struct command *commands;

    awoke_list _head;
};

struct cmder_transceiver {
    err_type (*recv)(struct cmder *, uint8_t *buf);
    err_type (*send)(struct cmder *, uint8_t *buf);
};

struct cmder {
    unsigned int nr_tbs;
    awoke_list command_tbs;
    struct cmder_transceiver *transceiver;
    unsigned int data_length;
    unsigned int datacome:1;
};

struct uartcmder {
    struct cmder base;
    unsigned int length;
    uint8_t buffer[1024];
};



#define cmder_burst(...) 	logm_burst(LOG_M_CMDER, 	__VA_ARGS__)
#define cmder_trace(...) 	logm_trace(LOG_M_CMDER, 	__VA_ARGS__)
#define cmder_debug(...) 	logm_debug(LOG_M_CMDER, 	__VA_ARGS__)
#define cmder_info(...) 	logm_info(LOG_M_CMDER, 	    __VA_ARGS__)
#define cmder_notice(...) 	logm_notice(LOG_M_CMDER, 	__VA_ARGS__)
#define cmder_err(...) 	    logm_err(LOG_M_CMDER, 		__VA_ARGS__)
#define cmder_warn(...) 	logm_warn(LOG_M_CMDER, 	    __VA_ARGS__)
#define cmder_bug(...) 	    logm_bug(LOG_M_CMDER, 		__VA_ARGS__)



#endif /* __CMDER_H__ */

