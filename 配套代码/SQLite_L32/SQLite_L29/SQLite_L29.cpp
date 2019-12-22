// SQLite_L29.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include "sqlite3.h"
#include <string>
#include <Windows.h>
using namespace std;

//int backupDb(
//	sqlite3* pDb,               /* Database to back up */
//	const char* zFilename,      /* Name of file to back up to */
//	const char* myMain,
//	int address_sqlite3_open,
//	int address_sqlite3_backup_init,
//	int address_sqlite3_backup_step,
//	int address_sqlite3_backup_remaining,
//	int address_sqlite3_backup_pagecount,
//	int address_sqlite3_sleep,
//	int address_sqlite3_backup_finish,
//	int address_sqlite3_errcode,
//	int address_sqlite3_close,
//	void(*xProgress)(int, int)  /* Progress function to invoke */
//);
int MakeBackupDBInMem();

void XProgress(int a, int b);

//函数指针
typedef int(__cdecl* Sqlite3_open)(
	const char*,
	sqlite3**
	);
typedef sqlite3_backup* (__cdecl* Sqlite3_backup_init)(
	sqlite3*,                     /* Database to write to */
	const char*,                  /* Name of database within pDestDb */
	sqlite3*,                      /* Database connection to read from */
	const char*                     /* Name of database within pSrcDb */
	);
typedef int(__cdecl* Sqlite3_backup_step)(sqlite3_backup*, int);
typedef int(__cdecl* Sqlite3_backup_remaining)(sqlite3_backup* );
typedef int(__cdecl* Sqlite3_backup_pagecount)(sqlite3_backup*);
typedef int(__cdecl* Sqlite3_sleep)(int);
typedef int(__cdecl* Sqlite3_backup_finish)(sqlite3_backup*);
typedef int(__cdecl* Sqlite3_errcode)(sqlite3*);
typedef int(__cdecl* Sqlite3_close)(sqlite3*);

typedef int(__cdecl* BackupDb)(
	sqlite3*,               /* Database to back up */
	const char*,      /* Name of file to back up to */
	const char*,
	int,
	int,
	int,
	int,
	int,
	int,
	int,
	int,
	int,
	void(*xProgress)(int, int)  /* Progress function to invoke */
);

int main()
{
	printf("Hello World!\n");
	//printf("backupDb地址：%p\n", backupDb);

	int address=MakeBackupDBInMem();
	printf("MakeBackupDBInMem地址：0x%08X\n", address);
	//return 0;

	sqlite3* db = NULL;
	int result = sqlite3_open("test.db", &db);

	const char* zFilename = "haha.db";
	int address_sqlite3_open = (int)&sqlite3_open;
	int address_sqlite3_backup_init = (int)&sqlite3_backup_init;
	int address_sqlite3_backup_step = (int)&sqlite3_backup_step;
	int address_sqlite3_backup_remaining = (int)&sqlite3_backup_remaining;
	int address_sqlite3_backup_pagecount = (int)&sqlite3_backup_pagecount;
	int address_sqlite3_sleep = (int)&sqlite3_sleep;
	int address_sqlite3_backup_finish = (int)&sqlite3_backup_finish;
	int address_sqlite3_errcode = (int)&sqlite3_errcode;
	int address_sqlite3_close = (int)&sqlite3_close;

	BackupDb p_BackupDb = (BackupDb)address;

	p_BackupDb(
		db,
		zFilename,
		"main",
		address_sqlite3_open,
		address_sqlite3_backup_init,
		address_sqlite3_backup_step,
		address_sqlite3_backup_remaining,
		address_sqlite3_backup_pagecount,
		address_sqlite3_sleep,
		address_sqlite3_backup_finish,
		address_sqlite3_errcode,
		address_sqlite3_close,
		XProgress);

	/*backupDb(
		db,
		zFilename,
		"main",
		address_sqlite3_open,
		address_sqlite3_backup_init,
		address_sqlite3_backup_step,
		address_sqlite3_backup_remaining,
		address_sqlite3_backup_pagecount,
		address_sqlite3_sleep,
		address_sqlite3_backup_finish,
		address_sqlite3_errcode,
		address_sqlite3_close,
		XProgress);*/

	sqlite3_close(db);
}

int MakeBackupDBInMem()
{
	string data = "55 8B EC 83 EC 30 8B 45 14 89 45 F0 8B 4D 18 89 4D EC 8B 55 1C 89 55 E8 8B 45 20 89 45 E0 8B 4D24 89 4D E4 8B 55 28 89 55 DC 8B 45 2C 89 45 D8 8B 4D 30 89 4D D4 8B 55 34 89 55 D0 8D 45 F4 508B 4D 0C 51 FF 55 F0 83 C4 08 89 45 FC 83 7D FC 00 0F 85 8D 00 00 00 8B 55 10 52 8B 45 08 50 8B4D 10 51 8B 55 F4 52 FF 55 EC 83 C4 10 89 45 F8 83 7D F8 00 74 61 6A 05 8B 45 F8 50 FF 55 E8 83C4 08 89 45 FC 8B 4D F8 51 FF 55 E4 83 C4 04 50 8B 55 F8 52 FF 55 E0 83 C4 04 50 FF 55 38 83 C408 83 7D FC 00 74 0C 83 7D FC 05 74 06 83 7D FC 06 75 08 6A 32 FF 55 DC 83 C4 04 83 7D FC 00 74B5 83 7D FC 05 74 AF 83 7D FC 06 74 A9 8B 45 F8 50 FF 55 D8 83 C4 04 8B 4D F4 51 FF 55 D4 83 C404 89 45 FC 8B 55 F4 52 FF 55 D0 83 C4 04 8B 45 FC 8B E5 5D C3";
	string space = " ";
	int pos = data.find(space);
	while (pos!=-1)
	{
		data.replace(pos,1,"");
		pos = data.find(space);
	}
	int codeLength = data.length()/2;
	unsigned char* code = new unsigned char[codeLength] {0};
	for (int i = 0; i < codeLength; i++)
	{
		string byteCode = data.substr(i*2,2);
		if (byteCode[0] >= '0' && byteCode[0] <= '9')
		{
			code[i] = (byteCode[0] - '0') * 16;
		}
		else
		{
			code[i] = (byteCode[0] - 'A'+10) * 16;
		}

		if (byteCode[1] >= '0' && byteCode[1] <= '9')
		{
			code[i] += (byteCode[1] - '0') ;
		}
		else
		{
			code[i] += (byteCode[1] - 'A' + 10);
		}
	}

	//申请内存空间，在内存中构造函数
	LPVOID lpAddressBackupDB = VirtualAlloc(NULL, codeLength, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (lpAddressBackupDB == NULL)
	{
		printf("VirtualAlloc error: %d\n", GetLastError());
		return 0;
	}
	CopyMemory(lpAddressBackupDB, code, codeLength);
	return (int)lpAddressBackupDB;
}

//int backupDb(
//	sqlite3* pDb,               /* Database to back up */
//	const char* zFilename,      /* Name of file to back up to */
//	const char* myMain,
//	int address_sqlite3_open,
//	int address_sqlite3_backup_init,
//	int address_sqlite3_backup_step,
//	int address_sqlite3_backup_remaining,
//	int address_sqlite3_backup_pagecount,
//	int address_sqlite3_sleep,
//	int address_sqlite3_backup_finish,
//	int address_sqlite3_errcode,
//	int address_sqlite3_close,
//	void(*xProgress)(int, int)  /* Progress function to invoke */
//) {
//	int rc;                     /* Function return code */
//	sqlite3* pFile;             /* Database connection opened on zFilename */
//	sqlite3_backup* pBackup;    /* Backup handle used to copy data */
//
//
//	Sqlite3_open p_Sqlite3_open = (Sqlite3_open)address_sqlite3_open;
//	Sqlite3_backup_init p_Sqlite3_backup_init = (Sqlite3_backup_init)address_sqlite3_backup_init;
//	Sqlite3_backup_step p_Sqlite3_backup_step = (Sqlite3_backup_step)address_sqlite3_backup_step;
//	Sqlite3_backup_remaining p_Sqlite3_backup_remaining = (Sqlite3_backup_remaining)address_sqlite3_backup_remaining;
//	Sqlite3_backup_pagecount p_Sqlite3_backup_pagecount = (Sqlite3_backup_pagecount)address_sqlite3_backup_pagecount;
//	Sqlite3_sleep p_Sqlite3_sleep = (Sqlite3_sleep)address_sqlite3_sleep;
//	Sqlite3_backup_finish p_Sqlite3_backup_finish = (Sqlite3_backup_finish)address_sqlite3_backup_finish;
//	Sqlite3_errcode p_Sqlite3_errcode = (Sqlite3_errcode)address_sqlite3_errcode;
//	Sqlite3_close p_Sqlite3_close = (Sqlite3_close)address_sqlite3_close;
//
//	rc = p_Sqlite3_open(zFilename, &pFile);
//
//	if (rc == SQLITE_OK) {
//
//		pBackup = p_Sqlite3_backup_init(pFile, myMain, pDb, myMain);
//		if (pBackup) {
//
//			do {
//				rc = p_Sqlite3_backup_step(pBackup, 5);
//
//				xProgress(
//					p_Sqlite3_backup_remaining(pBackup),
//					p_Sqlite3_backup_pagecount(pBackup)
//				);
//				if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
//					
//					p_Sqlite3_sleep(50);
//				}
//			} while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);
//
//			(void)p_Sqlite3_backup_finish(pBackup);
//		}
//		
//		rc = p_Sqlite3_errcode(pFile);
//	}
//
//	(void)p_Sqlite3_close(pFile);
//	return rc;
//}

void XProgress(int a, int b)
{
	printf("%d,%d\n",a,b);
}