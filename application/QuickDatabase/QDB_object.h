#ifndef __QDB_OBJECT_H__
#define __QDB_OBJECT_H__


#include "QDB.h"


#define __QDB_VERSION__		"1.0"




/* -- Object define -- {*/
/*
 * oid: QDB_OID_ROOT 1
 * name: Root
 * category: Static
 * description: Root Object.
 */
typedef struct _QDBObjectRoot {
} QDBObjectRoot;

/*} -- Object define -- */



/* -- Object define -- {*/
/*
 * oid: QDB_OID_DEV_INFO 2
 * name: DevInfo
 * category: Static
 * description: Device Information.
 */
typedef struct _QDBObjectDevInfo {
	char * Manufactory;		/* The manufacturer of the device. */
	char * ModelNumber;		/* The model number of the device. */
	char * SerialNumber;		/* The serial number of the device. */
	char * SoftwareVersion;		/* The software version of the device. */
	char * HardwareVersion;		/* The hardware version of the device. */
	char * BackupSwVersion;		/* The backup software version of the device. */
} QDBObjectDevInfo;

/*} -- Object define -- */



/* -- Object define -- {*/
/*
 * oid: QDB_OID_DEV_STATUS 3
 * name: DevStatus
 * category: Static
 * description: Device Status.
 */
typedef struct _QDBObjectDevStatus {
	int32_t Temperature;		/* The temperature of the device. */
	uint32_t Voltage;		/* The voltage of the device. */
	char * DateTime;		/* The datetime of the device. */
	uint32_t UpTime;		/* The uptime of the device. Represents the number of seconds the system has been running since booting. */
} QDBObjectDevStatus;

/*} -- Object define -- */



/* -- Object define -- {*/
/*
 * oid: QDB_OID_STATISTIC 4
 * name: Statistic
 * category: Static
 * description: Statistical summary.
 */
typedef struct _QDBObjectStatistic {
	uint32_t TXpackets;		/* Number of packets sent successfully. */
	uint32_t TXErrpackets;		/* Total packages failed to send. */
	uint32_t RXpackets;		/* Number of packets receive successfully. */
	uint32_t TXBytes;		/* Total size of bytes sent successfully. */
	uint32_t RXBytes;		/* Total size of bytes receive successfully. */
} QDBObjectStatistic;

/*} -- Object define -- */



/* -- Object define -- {*/
/*
 * oid: QDB_OID_MODEM 5
 * name: Modem
 * category: Static
 * description: Modem information.
 */
typedef struct _QDBObjectModem {
	uint32_t TXpower;		/* RF transmitting power. */
	uint32_t PhysicalCellID;		/* Physical cell ID. */
} QDBObjectModem;

/*} -- Object define -- */



/* -- Object define -- {*/
/*
 * oid: QDB_OID_TEST_A 6
 * name: TestA
 * category: Static
 * description: QDB Struct Test.
 */
typedef struct _QDBObjectTestA {
	uint32_t Testbit;		/* Testbit. */
} QDBObjectTestA;

/*} -- Object define -- */



/* -- Object define -- {*/
/*
 * oid: QDB_OID_TEST_B 7
 * name: TestB
 * category: Static
 * description: QDB Struct Test.
 */
typedef struct _QDBObjectTestB {
	uint32_t Testbit;		/* Testbit. */
} QDBObjectTestB;

/*} -- Object define -- */



#endif /* __QDB_OBJECT_H__ */