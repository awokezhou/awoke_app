#include "QDB.h"


QDBObjectMember DevInfoObjectMembers[] = {
	{
		"Manufactory",
		QDB_MT_STRING,
		QDB_MF_RO,
		0,
		"",
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"ModelNumber",
		QDB_MT_STRING,
		QDB_MF_RO,
		0,
		"",
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"SerialNumber",
		QDB_MT_STRING,
		QDB_MF_RO,
		0,
		"",
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"SoftwareVersion",
		QDB_MT_STRING,
		QDB_MF_RO,
		0,
		"",
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"HardwareVersion",
		QDB_MT_STRING,
		QDB_MF_RO,
		0,
		"",
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"BackupSwVersion",
		QDB_MT_STRING,
		QDB_MF_RO,
		0,
		"",
		{(void *)0, (void *)0}  /* validator data */
	},
};

QDBObject DevInfoObject = {
	2, /* oid:QDB_OID_DEV_INFO */
	"DevInfo", /* object name */
	0,0, /* flags and instance depth */
	NULL, /* parent */
	sizeof(DevInfoObjectMembers)/sizeof(QDBObjectMember), /* members number */
	DevInfoObjectMembers, /* point to members */
	0, /* members number */
	NULL, /* point to members */
};

QDBObjectMember DevStatusObjectMembers[] = {
	{
		"Temperature",
		QDB_MT_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"Voltage",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"DateTime",
		QDB_MT_DATETIME,
		QDB_MF_WR,
		0,
		"",
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"UpTime",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
};

QDBObject DevStatusObject = {
	3, /* oid:QDB_OID_DEV_STATUS */
	"DevStatus", /* object name */
	0,0, /* flags and instance depth */
	NULL, /* parent */
	sizeof(DevStatusObjectMembers)/sizeof(QDBObjectMember), /* members number */
	DevStatusObjectMembers, /* point to members */
	0, /* members number */
	NULL, /* point to members */
};

QDBObjectMember StatisticObjectMembers[] = {
	{
		"TXpackets",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"TXErrpackets",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"RXpackets",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"TXBytes",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"RXBytes",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
};

QDBObject StatisticObject = {
	4, /* oid:QDB_OID_STATISTIC */
	"Statistic", /* object name */
	0,0, /* flags and instance depth */
	NULL, /* parent */
	sizeof(StatisticObjectMembers)/sizeof(QDBObjectMember), /* members number */
	StatisticObjectMembers, /* point to members */
	0, /* members number */
	NULL, /* point to members */
};

QDBObjectMember ModemObjectMembers[] = {
	{
		"TXpower",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
	{
		"PhysicalCellID",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
};

QDBObject ModemObject = {
	5, /* oid:QDB_OID_MODEM */
	"Modem", /* object name */
	0,0, /* flags and instance depth */
	NULL, /* parent */
	sizeof(ModemObjectMembers)/sizeof(QDBObjectMember), /* members number */
	ModemObjectMembers, /* point to members */
	0, /* members number */
	NULL, /* point to members */
};

QDBObjectMember TestBObjectMembers[] = {
	{
		"Testbit",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
};

QDBObject TestBObject = {
	7, /* oid:QDB_OID_TEST_B */
	"TestB", /* object name */
	0,0, /* flags and instance depth */
	NULL, /* parent */
	sizeof(TestBObjectMembers)/sizeof(QDBObjectMember), /* members number */
	TestBObjectMembers, /* point to members */
	0, /* members number */
	NULL, /* point to members */
};

QDBObjectMember TestAObjectMembers[] = {
	{
		"Testbit",
		QDB_MT_UNSIGNED_INTEGER,
		QDB_MF_RO,
		0,
		0,
		{(void *)0, (void *)0}  /* validator data */
	},
};

QDBObject TestAChilds[] = {
	{
		7,
		"TestB", /* name:Root.TestA.TestB */
		0,0, /* flags and instance depth */
		NULL, /* parent */
		sizeof(TestBObjectMembers)/sizeof(QDBObjectMember), /* members number */
		TestBObjectMembers, /* point to members */
		0, /* members number */
		NULL, /* point to members */
	},
};

QDBObject TestAObject = {
	6, /* oid:QDB_OID_TEST_A */
	"TestA", /* object name */
	0,0, /* flags and instance depth */
	NULL, /* parent */
	sizeof(TestAObjectMembers)/sizeof(QDBObjectMember), /* members number */
	TestAObjectMembers, /* point to members */
	sizeof(TestAChilds)/sizeof(QDBObject), /* childs number */
	TestAChilds, /* point to childs */
};

QDBObject RootChilds[] = {
	{
		2,
		"DevInfo", /* name:Root.DevInfo */
		0,0, /* flags and instance depth */
		NULL, /* parent */
		sizeof(DevInfoObjectMembers)/sizeof(QDBObjectMember), /* members number */
		DevInfoObjectMembers, /* point to members */
		0, /* members number */
		NULL, /* point to members */
	},
	{
		3,
		"DevStatus", /* name:Root.DevStatus */
		0,0, /* flags and instance depth */
		NULL, /* parent */
		sizeof(DevStatusObjectMembers)/sizeof(QDBObjectMember), /* members number */
		DevStatusObjectMembers, /* point to members */
		0, /* members number */
		NULL, /* point to members */
	},
	{
		4,
		"Statistic", /* name:Root.Statistic */
		0,0, /* flags and instance depth */
		NULL, /* parent */
		sizeof(StatisticObjectMembers)/sizeof(QDBObjectMember), /* members number */
		StatisticObjectMembers, /* point to members */
		0, /* members number */
		NULL, /* point to members */
	},
	{
		5,
		"Modem", /* name:Root.Modem */
		0,0, /* flags and instance depth */
		NULL, /* parent */
		sizeof(ModemObjectMembers)/sizeof(QDBObjectMember), /* members number */
		ModemObjectMembers, /* point to members */
		0, /* members number */
		NULL, /* point to members */
	},
	{
		6,
		"TestA", /* name:Root.TestA */
		0,0, /* flags and instance depth */
		NULL, /* parent */
		sizeof(TestAObjectMembers)/sizeof(QDBObjectMember), /* members number */
		TestAObjectMembers, /* point to members */
		sizeof(TestAChilds)/sizeof(QDBObject), /* childs number */
		TestAChilds, /* point to childs */
	},
};

QDBObject RootObject = {
	1, /* oid:QDB_OID_ROOT */
	"Root", /* object name */
	0,0, /* flags and instance depth */
	NULL, /* parent */
	0, /* members number */
	NULL, /* point to members */
	sizeof(RootChilds)/sizeof(QDBObject), /* childs number */
	RootChilds, /* point to childs */
};

