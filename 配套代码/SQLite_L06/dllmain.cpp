// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "resource.h"
#include <iostream>
#include <string>
using namespace std;

VOID ShowDemoUI(HMODULE hModule);
INT_PTR CALLBACK DialogProc(_In_ HWND   hwndDlg, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
void Hook();
void UnHook();
VOID HookFun();
void GetData(int a);
BYTE HookData[5];
int HookAddress = 0xA177D;
int JumpBackAddress = HookAddress + 5;
HWND HwndDlg;

//DLL入口函数
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		HANDLE hANDLE = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShowDemoUI, hModule, NULL, 0);
		if (hANDLE != 0)
		{
			CloseHandle(hANDLE);
		}
		break;		
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
//显示操作的窗口
VOID ShowDemoUI(HMODULE hModule)
{
	DialogBox(hModule, MAKEINTRESOURCE(IDD_MAIN), NULL, &DialogProc);
}

// 窗口回调函数，处理窗口事件
INT_PTR CALLBACK DialogProc(_In_ HWND   hwndDlg, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	HwndDlg = hwndDlg;
	switch (uMsg)
	{
	case WM_CLOSE:
		//关闭窗口事件
		EndDialog(hwndDlg, 0);
		break;
	case WM_COMMAND:
		//调用函数
		if (wParam == IDC_BUTTON_RUN)
		{
			__asm
			{
				mov eax, 0x000A11E0
				call eax
			}
			break;
		}

		//IDC_REFRESH
		if (wParam == IDC_CHECK_GET)
		{
			HWND child = GetDlgItem(hwndDlg, IDC_CHECK_GET);
			int isChecked = (int)SendMessage(child, BM_GETCHECK, 0, 0);

			//未选中，不截取数据
			if (isChecked == 0)
			{
				UnHook();
				std::cout << "未选中.\n";
			}
			else
			{
				Hook();				
				std::cout << "已选中，准备截取数据...\n";
			}
			break;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

void Hook()
{
	memcpy(&HookData, (BYTE*)HookAddress, 5);

	BYTE jmpCode[5] = { 0 };
	jmpCode[0] = 0xE9;
	*(DWORD*)&jmpCode[1] = (DWORD)HookFun - HookAddress - 5;

	WriteProcessMemory(GetCurrentProcess(), (LPVOID)HookAddress, jmpCode, 5, 0);
}

void UnHook()
{
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)HookAddress, &HookData, 5, 0);
}

__declspec(naked) VOID HookFun()
{
	//保存现场
	__asm
	{
		//补充被覆盖的代码
		mov eax, 0x000AA138
		mov eax,[eax]

		//保存寄存器
		pushad
		pushf

		//调用函数
		push eax
		call GetData
		add esp,4

		//恢复寄存器
		popf
		popad

		//跳回被HOOK指令的下一条指令
		jmp JumpBackAddress
	}
}

void GetData(int a)
{
	std::cout << "截获的数据：" << a << "\n";

	HWND child = GetDlgItem(HwndDlg, IDC_EDIT1);
	WCHAR szString[1024] = {0};
	GetDlgItemText(HwndDlg, IDC_EDIT1, szString, 1024);

	wstring myString(szString);
	myString.append(L"截获的数据：");
	myString.append(to_wstring(a));
	myString.append(L"\r\n");

	SetDlgItemText(HwndDlg, IDC_EDIT1, myString.c_str());
}

