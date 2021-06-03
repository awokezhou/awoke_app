
#include "bk_strucp.h"
#include "benchmark.h"


struct modnode {
    uint32_t mod;
    awoke_list _head;
};

static void strucp_node_find(uint32_t mod, awoke_list *head)
{
    struct modnode *p;
    
    list_foreach(p, head, _head) {
        if (p->mod == mod) {
            bk_notice("repeat mod:%d", mod);
            break;
        }   
    }
}

err_type benchmark_strucp_test(int argc, char *argv[])
{
    int i;
    uint32_t mod;
    uint32_t mod_value = 113;    /* 61, 83, 113, 151, 211, 281, 379, 509 683 */
    uint32_t baseaddr = 0x20000000;
    struct modnode *p, *temp;

    awoke_list search;

    list_init(&search);
    

    for (i=0; i<128; i++) {
        struct modnode *node = mem_alloc_z(sizeof(struct modnode));
        node->mod = baseaddr%mod_value;
        strucp_node_find(node->mod, &search);
        list_append(&node->_head, &search);
        bk_debug("addr:0x%x mod:%d", baseaddr, node->mod);
        baseaddr += 4;
    }

    list_for_each_entry_safe(p, temp, &search, _head) {
        list_unlink(&p->_head);
        mem_free(p);
    }
    
    return et_ok;
}
