// REV_L01.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>

int a = 0;

void f()
{
	printf("\n");
	for (size_t i = 0; i < 3; i++)
	{
		a++;
		printf("%d\n",a);
	}
}


int main()
{
   printf( "Hello World!\n");
   printf("%p\n", f);
   f();
   getchar();
}
