
#ifndef __QDB_H__
#define __QDB_H__


#include "awoke_type.h"
#include "awoke_error.h"



/* -- Member Flag --{*/
/* Parameter Access Control(PAC) */
#define QDB_MF_RO   0x01
#define QDB_MF_WO   0x02
#define QDB_MF_WR   (QDB_MF_RO|QDB_MF_WO)
/*}-- Member Flag -- */


#define QDB_NOTIFICATION_VALUE    0
#define QDB_ACCESS_BITMASK  0


/* -- Member Type --{*/
typedef enum {
    QDB_MT_STRING,
	QDB_MT_INTEGER,
	QDB_MT_UNSIGNED_INTEGER,
    QDB_MT_DATETIME,
	
} QDBMemberType;
/*}-- Member Type -- */


/* -- Object --{*/
typedef struct _QDBObjectMember {
    const char *name;                   /* This name of member */
    struct _QDBObjectMember *parent;    /* Parent point */
    QDBMemberType dtype;
    uint16_t flags;
    uint16_t offset;
    char *defvalue;
    char *sugvalue;
    void *max;
    void *min;
} QDBObjectMember;

typedef struct _QDBObject {
    uint32_t oid;
    const char *name;
    uint8_t flags;
    uint8_t depth;
    struct _QDBObject *parent;
    uint16_t memeber_nr;
    void *member;
    uint16_t child_nr;
    struct _QDBObject *child;
} QDBObject;
/*}-- Object -- */

#endif /* __QDB_H__ */