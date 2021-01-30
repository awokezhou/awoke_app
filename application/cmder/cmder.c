
#include "cmder.h"
#include "awoke_memory.h"
#include <unistd.h>


static err_type ecmd_handler_dfg(struct command *cmd);
static bool ecmds_format_check(void *buf, int len);
static bool ecmds_match(struct command_table *tb, void *buf, int len, struct command **cmd);



struct ecmd_struct {
    uint8_t code;
    unsigned int need;
};

#define ecmd_match_make(e)     {MATCH_STRUCT, 1, .val.struc=(void*)&e}


struct ecmd_struct ecmds[] = {
    {0x11, 6},
};

static struct command test_struct_commands[] = {
    {ecmd_match_make(ecmds[0]), NULL, NULL, ecmd_handler_dfg},
};

static struct command_matcher test_struct_matcher = {
    .format = ecmds_format_check,
    .match = ecmds_match,
};

static err_type ecmd_handler_dfg(struct command *cmd)
{
    cmder_notice("dfg");
    return et_ok;
}

static bool ecmds_format_check(void *buf, int len)
{
    bool find = FALSE;
    uint8_t *head = (uint8_t *)buf;
    uint8_t *pos = head;
    uint16_t prefix;
    
    if (len < 6) {
        cmder_err("length too small");
        return FALSE;
    }

    prefix = (head[0]<<8) | head[1];
    if (prefix != 0x8101) {
        cmder_err("prefix error");
        return FALSE;
    }

    cmder_trace("prefix ok");

    while (pos++ != (head+len)) {
        if (*pos == 0xff) {
            find = TRUE;
            break;
        }
    }

    if (!find) {
        cmder_err("postfix not find");
        return FALSE;
    }

    cmder_trace("postfix ok");

    return TRUE;
}

static bool ecmds_match(struct command_table *tb, void *buf, int len, struct command **cmd)
{
    struct command *p;
    struct ecmd_struct *m;
    uint8_t *pos = (uint8_t *)buf;

    cmder_trace("ecmds match in len:%d", len);

    awoke_hexdump_trace(pos, len);
    
    array_foreach_start(tb->commands, tb->nr_commands, p) {
        m = (struct ecmd_struct *)p->match.val.struc;
        cmder_trace("code:%d need:%d", m->code, m->need);
        if ((m->code == pos[2]) && (m->need == len)) {
            *cmd = p;
            cmder_debug("find");
            return TRUE;
        }
        
    } array_foreach_end();

    
    return FALSE;
}

static err_type ecmd_recv(struct cmder *c, uint8_t *buf)
{
    uint8_t cmd[6] = {0x81, 0x01, 0x11, 0x01, 0x02, 0xff};
   memcpy(buf, cmd, 6);
    c->datacome = 1;
    c->data_length = 6;
    return et_ok;
}

err_type cmder_tb_register(struct cmder *c, const char *name, struct command *commands, unsigned int size, 
    struct command_matcher *matcher)
{
    struct command_table *p, *n;
        
    if (!c || !name || !commands || !size || !matcher) {
        cmder_err("param error");
        return et_param;
    }

    list_for_each_entry(p, &c->command_tbs, _head) {
        if (!strcmp(p->name, name)) {
            return et_exist;
        }
    }

    n = mem_alloc_z(sizeof(struct command_table));
    n->name = awoke_string_dup(name);
    n->commands = commands;
    n->nr_commands = size;
    n->matcher = matcher;

    list_append(&n->_head, &c->command_tbs);

    return et_ok;
}

static err_type cmder_init(struct uartcmder *cmder)
{
    struct cmder_transceiver *transceiver;

    list_init(&cmder->base.command_tbs);
    
    cmder_tb_register(&cmder->base, "ecmd-test", test_struct_commands, 
        array_size(test_struct_commands), &test_struct_matcher);

    cmder_debug("tb register");

    transceiver = mem_alloc_z(sizeof(struct cmder_transceiver));
    transceiver->recv = ecmd_recv;
    transceiver->send = NULL;

    cmder->base.transceiver = transceiver;
    
    return et_ok;
}

static err_type cmder_command_recv(struct uartcmder *cmder)
{
    cmder->base.transceiver->recv(&cmder->base, cmder->buffer);
    cmder_trace("recv %d", cmder->base.data_length);
    return et_ok;
}

static struct command *commnad_get(struct cmder *c, void *buffer, unsigned int length)
{
    struct command *cmd;
    struct command_table *tb;
    struct command_matcher *matcher;

    list_for_each_entry(tb, &c->command_tbs, _head) {

        cmder_debug("tab:%s", tb->name);
        matcher = tb->matcher;

        if (matcher->format) {
            cmder_trace("format check");
            if (!matcher->format(buffer, length)) {
                continue;
            }
            cmder_trace("format check ok");
        }

        if (matcher->match(tb, buffer, length, &cmd)) {
            return cmd;
        }
    }

    return NULL;
}

static err_type cmder_command_exec(struct uartcmder *cmder)
{
    err_type ret;
    struct cmder *c = &cmder->base;
    struct command *cmd;
    
    if (!c->datacome) {
        cmder_trace("no data");
        return et_ok;
    }

    cmd = commnad_get(c, cmder->buffer, c->data_length);
    if (!cmd) {
        return ret;
    }

    ret = cmd->exe(cmd);
    if (ret != et_ok) {
        return ret;
    }

    return et_ok;
}

static err_type cmder_command_send(struct uartcmder *cmder)
{
    return et_ok;
}

int main(int argc, char *argv[])
{
    struct uartcmder *cmder;

    cmder = mem_alloc_z(sizeof(struct uartcmder));
    cmder_debug("cmder alloc ok");
    
    cmder_init(cmder);

    cmder_debug("cmder init ok");

    while (1) {
        cmder_command_recv(cmder);
        cmder_command_exec(cmder);
        cmder_command_send(cmder);
        sleep(2);
    }

    return 0;
}
