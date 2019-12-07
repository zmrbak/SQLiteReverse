// SQLite_20.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
typedef int (*sqlite3_callback)(int, int, int, int);

int index = 0;

//易语言中的函数调用方式
int __stdcall e_f(int a,int b,int c,int d)
{
	index++;
	printf("e_f: %d,%d,%d,%d,%d\n", index, a, b, c, d);
	return 0;
}

int __cdecl c_f(int a, int b, int c, int d)
{
	return e_f(a,b,c,d);
}

int sqlite3_exec(int db,int sql, sqlite3_callback callback,int argument,int errmsg)
{
	for (size_t i = 0; i < 5; i++)
	{
		callback(1, 2, 3, 4);
	}
	return 0;
}

int main()
{
    printf("Hello World!\n");
	sqlite3_exec(0,0, c_f,0,0);
}
