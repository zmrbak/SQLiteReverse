// SQLite_L29.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include "sqlite3.h"

int backupDb(
	sqlite3* pDb,               /* Database to back up */
	const char* zFilename,      /* Name of file to back up to */
	void(*xProgress)(int, int)  /* Progress function to invoke */
);

void XProgress(int a, int b);

int main()
{
	printf("Hello World!\n");
	sqlite3* db = NULL;
	int result = sqlite3_open("test.db", &db);

	const char* zFilename = "haha.db";
	backupDb(db, zFilename, XProgress);

	sqlite3_close(db);
}

int backupDb(
	sqlite3* pDb,               /* Database to back up */
	const char* zFilename,      /* Name of file to back up to */
	void(*xProgress)(int, int)  /* Progress function to invoke */
) {
	int rc;                     /* Function return code */
	sqlite3* pFile;             /* Database connection opened on zFilename */
	sqlite3_backup* pBackup;    /* Backup handle used to copy data */

	/* Open the database file identified by zFilename. */
	rc = sqlite3_open(zFilename, &pFile);
	if (rc == SQLITE_OK) {

		/* Open the sqlite3_backup object used to accomplish the transfer */
		pBackup = sqlite3_backup_init(pFile, "main", pDb, "main");
		if (pBackup) {

			/* Each iteration of this loop copies 5 database pages from database
			** pDb to the backup database. If the return value of backup_step()
			** indicates that there are still further pages to copy, sleep for
			** 250 ms before repeating. */
			do {
				rc = sqlite3_backup_step(pBackup, 5);
				xProgress(
					sqlite3_backup_remaining(pBackup),
					sqlite3_backup_pagecount(pBackup)
				);
				if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
					sqlite3_sleep(250);
				}
			} while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

			/* Release resources allocated by backup_init(). */
			(void)sqlite3_backup_finish(pBackup);
		}
		rc = sqlite3_errcode(pFile);
	}

	/* Close the database connection opened on database file zFilename
	** and return the result of this function. */
	(void)sqlite3_close(pFile);
	return rc;
}

void XProgress(int a, int b)
{
	printf("%d,%d\n",a,b);
}