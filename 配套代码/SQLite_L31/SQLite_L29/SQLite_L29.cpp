// SQLite_L29.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include "sqlite3.h"

int backupDb(
	sqlite3* pDb,               /* Database to back up */
	const char* zFilename,      /* Name of file to back up to */
	const char* myMain,
	int address_sqlite3_open,
	int address_sqlite3_backup_init,
	int address_sqlite3_backup_step,
	int address_sqlite3_backup_remaining,
	int address_sqlite3_backup_pagecount,
	int address_sqlite3_sleep,
	int address_sqlite3_backup_finish,
	int address_sqlite3_errcode,
	int address_sqlite3_close,
	void(*xProgress)(int, int)  /* Progress function to invoke */
);

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

int main()
{
	printf("Hello World!\n");
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
	backupDb(
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

	sqlite3_close(db);
}

int backupDb(
	sqlite3* pDb,               /* Database to back up */
	const char* zFilename,      /* Name of file to back up to */
	const char* myMain,
	int address_sqlite3_open,
	int address_sqlite3_backup_init,
	int address_sqlite3_backup_step,
	int address_sqlite3_backup_remaining,
	int address_sqlite3_backup_pagecount,
	int address_sqlite3_sleep,
	int address_sqlite3_backup_finish,
	int address_sqlite3_errcode,
	int address_sqlite3_close,
	void(*xProgress)(int, int)  /* Progress function to invoke */
) {
	int rc;                     /* Function return code */
	sqlite3* pFile;             /* Database connection opened on zFilename */
	sqlite3_backup* pBackup;    /* Backup handle used to copy data */


	Sqlite3_open p_Sqlite3_open = (Sqlite3_open)address_sqlite3_open;
	Sqlite3_backup_init p_Sqlite3_backup_init = (Sqlite3_backup_init)address_sqlite3_backup_init;
	Sqlite3_backup_step p_Sqlite3_backup_step = (Sqlite3_backup_step)address_sqlite3_backup_step;
	Sqlite3_backup_remaining p_Sqlite3_backup_remaining = (Sqlite3_backup_remaining)address_sqlite3_backup_remaining;
	Sqlite3_backup_pagecount p_Sqlite3_backup_pagecount = (Sqlite3_backup_pagecount)address_sqlite3_backup_pagecount;
	Sqlite3_sleep p_Sqlite3_sleep = (Sqlite3_sleep)address_sqlite3_sleep;
	Sqlite3_backup_finish p_Sqlite3_backup_finish = (Sqlite3_backup_finish)address_sqlite3_backup_finish;
	Sqlite3_errcode p_Sqlite3_errcode = (Sqlite3_errcode)address_sqlite3_errcode;
	Sqlite3_close p_Sqlite3_close = (Sqlite3_close)address_sqlite3_close;

	rc = p_Sqlite3_open(zFilename, &pFile);

	if (rc == SQLITE_OK) {

		pBackup = p_Sqlite3_backup_init(pFile, myMain, pDb, myMain);
		if (pBackup) {

			do {
				rc = p_Sqlite3_backup_step(pBackup, 5);

				xProgress(
					p_Sqlite3_backup_remaining(pBackup),
					p_Sqlite3_backup_pagecount(pBackup)
				);
				if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
					
					p_Sqlite3_sleep(50);
				}
			} while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

			(void)p_Sqlite3_backup_finish(pBackup);
		}
		
		rc = p_Sqlite3_errcode(pFile);
	}

	(void)p_Sqlite3_close(pFile);
	return rc;
}

void XProgress(int a, int b)
{
	printf("%d,%d\n",a,b);
}