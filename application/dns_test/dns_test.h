
#ifndef __DNS_TEST_H__
#define __DNS_TEST_H__

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_socket.h"
#include "awoke_package.h"

#define DNS_PORT                     	0x35
#define DNS_ONE_QUESTION                0x01

#define DNS_NAME_OFFSET               	0xc0

#define DNS_F_QUERY_RESPONSE			0x8000
#define DNS_F_OPERATION_CODE     		0x7800 
#define DNS_F_TRUNCATION          		0x0200
#define DNS_F_RESPONSE_CODE      		0x000f
#define DNS_F_QUERY               		0x0100 /* Standard query. */
#define DNS_TYPE_IPV4                   0x0001 /* A record (host address. */
#define DNS_CLASS                       0x0001 /* IN */
#define DNS_RX_F_MASK                	0x800f /* The bits of interest in the flags field of incoming DNS messages. */
#define DNS_EXPECTED_RX_F            	0x8000 /* Should be a response, without any errors. */
#define DNS_TYPE_IPV6                   0x001C

typedef enum {
	DNS_T_A = 1,		/* a host address */
	DNS_T_NS,       		/* an authoritative name server */
	DNS_T_MD,       		/* a mail destination (Obsolete - use MX) */
	DNS_T_MF,       		
	DNS_T_CNAME,    		/* the canonical name for an alias */
	DNS_T_SOA,      		/* marks the start of a zone of authority  */
	DNS_T_MB,       		/* a mailbox domain name (EXPERIMENTAL) */
	DNS_T_MG,       
	DNS_T_MR,       
	DNS_T_NUL,
	DNS_T_WKS,      		/* a well known service description */
	DNS_T_PTR,      		/* a domain name pointer */
	DNS_T_HINFO,    		/* host information */
	DNS_T_MINFO,    		/* mailbox or mail list information */
	DNS_T_MX,       		/* mail exchange */
	DNS_T_TXT,      		/* text strings */

	DNS_AAA = 0x1c, 	/* IPv6 A */
	DNS_SRVRR = 0x21  	/* RFC 2782: location of services */
} dns_type;

typedef enum {
	DNS_C_IN = 1,				/* the Internet */
	DNS_C_CS,
	DNS_C_CH,
	DNS_C_HS
} dns_class;

typedef enum {
	DNS_OPT_QUERY,
	DNS_OPT_IQUERY,
	DNS_OPT_STATUS
} dns_opt;

typedef struct _dns_rr {
#define DNS_NAME_SIZE 255
  char name[DNS_NAME_SIZE];
  uint16_t type;
  uint16_t class;
  uint32_t ttl;
  uint16_t rdatalen;
  char data[DNS_NAME_SIZE];
} dns_rr;

typedef struct _dns_header {
	uint16_t transaction_id;
    uint16_t flags;
    uint16_t questions;			/* carries the query name and other query parameters. */
    uint16_t answers_RRs;		/* carries RRs which directly answer the query */
    uint16_t authority_RRs;		/* carries RRs which describe other authoritative servers.
    							   May optionally carry the SOA RR for the authoritative
    							   data in the answer section */
    uint16_t additional_RRs;	/* carries RRs which may be helpful in using the RRs in the
							       other sections */
} dns_header; 

typedef struct _dns_message {
	dns_header header;
#define DNS_RR_NR	5
	dns_rr question[DNS_RR_NR];
	dns_rr answer[DNS_RR_NR];
} dns_message;

typedef struct _dns_request {
	int host_nr;
	int addr_valid;
	mem_ptr_t host[DNS_RR_NR];
	dns_message message;
	uint16_t us_record_type;
	uint16_t us_class;
	int original_len;
#define DNS_REQUEST_LEN	512
	uint8_t original_buf[DNS_REQUEST_LEN];
} dns_request;

typedef struct _dns_server_entry {
	const char *addr;
	awoke_list _head;
} dns_server_entry;
				
typedef struct _dns_entry {
	char hostname[DNS_NAME_SIZE];
	uint8_t ipaddr[4];
	awoke_list _head;
} dns_entry;

#endif /* __DNS_TEST_H__ */
