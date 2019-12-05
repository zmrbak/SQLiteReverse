// SQLite_17.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
int  f___(int a, int b, int c, int d)
{
	//printf("Hello World!\n");
	return 0;
}


int __cdecl f___cdecl(int a,int b,int c,int d)
{
	//printf("Hello World!\n");
	return 0;
}

int __stdcall f__stdcall(int a, int b, int c, int d)
{
	//printf("Hello World!\n");
	return 0;
}
int __fastcall f__fastcall(int a, int b, int c, int d)
{
	//printf("Hello World!\n");
	return 0;
}

int __vectorcall f__vectorcall(int a, int b, int c, int d)
{
	//printf("Hello World!\n");
	return 0;
}

__declspec(naked) int f__naked(int a, int b, int c, int d)
{
	//printf("Hello World!\n");
	__asm
	{
		mov eax,0
	}
}
int main()
{
	//printf("Hello World!\n");
	f___(1, 2, 3, 4);
	f___cdecl(1,2,3,4);
	f__stdcall(1, 2, 3, 4);
	f__fastcall(1, 2, 3, 4);
	f__vectorcall(1, 2, 3, 4);
	f__naked(1, 2, 3, 4);
}
