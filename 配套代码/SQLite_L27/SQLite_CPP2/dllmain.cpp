// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "resource.h"
#include "shellapi.h"

#include <string>
#include <strstream>
#include <list>

#pragma comment(lib, "Version.lib")
using namespace std;

VOID ShowDemoUI(HMODULE hModule);
INT_PTR CALLBACK DialogProc(_In_ HWND   hwndDlg, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
BOOL IsWxVersionValid();
VOID UnInject();
VOID HookWx();
VOID InlinkHookJump();
VOID OutPutData(int dbAddress, int dbHandle);
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID ReBootWeChat();
VOID RunSQL();
VOID AddLog(string text);
typedef int (__cdecl* sqlite3_callback)(void*, int, char**, char**);
typedef int(__cdecl* Sqlite3_exec)(
	DWORD,                /* The database on which the SQL executes */
	const char*,           /* The SQL to be executed */
	sqlite3_callback, /* Invoke this callback routine */
	void*,                 /* First argument to xCallback() */
	char**             /* Write error messages here */
	);
int __cdecl MyCallback(void* para, int nColumn, char** colValue, char** colName);//回调函数

DWORD wxBaseAddress = 0;
HWND hWinDlg;
const string wxVersoin = "2.6.8.65";
BOOL isWxHooked = FALSE;
DWORD hookAddress = 0;
DWORD overWritedCallAdd = 0;
DWORD jumpBackAddress = 0;
DWORD dwTimeId = 0;

//定义一个结构体来存储 数据库句柄-->数据库名
struct DbNameHandle
{
	int DBHandler;
	char DBName[MAX_PATH];
};

//在内存中存储一个“数据库句柄-->数据库名”的链表，
list<DbNameHandle> dbList;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		//##########################################################
		//
		//注意：仅适配PC微信2.6.8.65版本，其它版本不可用
		//
		//##########################################################

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
	//获取WeChatWin.dll的基址
	while (wxBaseAddress == 0)
	{
		wxBaseAddress = (DWORD)GetModuleHandle(TEXT("WeChatWin.dll"));
		Sleep(100);
	}

	if (IsWxVersionValid() == FALSE)
	{
		MessageBoxA(NULL, "微信版本不匹配，请使用2.6.8.65版本！", "错误", MB_OK);
		UnInject();
		return;
	}

	//在登录微信前注入
	int* loginStatus = (int*)(wxBaseAddress + 0x126D840 + 0x194);
	if (*loginStatus != 0)
	{
		if (IDOK == MessageBox(NULL, TEXT("微信已经登录，该功能无法使用！\n是否重新启动微信？"), TEXT("错误"), MB_OKCANCEL | MB_ICONERROR))
		{
			ReBootWeChat();
		}
		UnInject();
		return;
	}

	//启动窗口
	DialogBox(hModule, MAKEINTRESOURCE(IDD_MAIN), NULL, &DialogProc);
}
//窗口回调函数，处理窗口事件
INT_PTR CALLBACK DialogProc(_In_ HWND   hwndDlg, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	hWinDlg = hwndDlg;
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//DLL加载后，就启动Inline HOOK
		HookWx();
		HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_SQL);
		string sql = "select * from sqlite_master";
		SendMessageA(edit, WM_SETTEXT, NULL, (LPARAM)(sql.c_str()));
		break;
	}
	case WM_CLOSE:
	{
		//关闭窗口事件
		UnInject();
		EndDialog(hwndDlg, 0);
		break;
	}
	case WM_COMMAND:
	{
		//执行SQL
		if (wParam == IDC_BUTTON_SQLRUN)
		{
			RunSQL();
			break;
		}

		//重启微信
		if (wParam == IDC_BUTTON_WX_REBOOT)
		{
			ReBootWeChat();
			break;
		}

		//《SQLite逆向分析》视频
		if (wParam == IDC_BUTTON_SQL_163)
		{
			ShellExecute(hwndDlg, TEXT("open"), TEXT("http://t.cn/AigE0KYQ"), NULL, NULL, NULL);
			break;
		}

		//《PC微信探秘》视频
		if (wParam == IDC_BUTTON_WX_163)
		{
			ShellExecute(hwndDlg, TEXT("open"), TEXT("http://t.cn/EXUbebQ"), NULL, NULL, NULL);
			break;
		}

		//《逆向分析入门》视频
		if (wParam == IDC_BUTTON_REV_163)
		{
			ShellExecute(hwndDlg, TEXT("open"), TEXT("http://t.cn/AiTKOm8F"), NULL, NULL, NULL);
			break;
		}
		break;
	}
	default:
		break;
	}
	return FALSE;
}

//检查微信版本是否匹配
BOOL IsWxVersionValid()
{
	WCHAR VersionFilePath[MAX_PATH];
	if (GetModuleFileName((HMODULE)wxBaseAddress, VersionFilePath, MAX_PATH) == 0)
	{
		return FALSE;
	}

	string asVer = "";
	VS_FIXEDFILEINFO* pVsInfo;
	unsigned int iFileInfoSize = sizeof(VS_FIXEDFILEINFO);
	int iVerInfoSize = GetFileVersionInfoSize(VersionFilePath, NULL);
	if (iVerInfoSize != 0) {
		char* pBuf = new char[iVerInfoSize];
		if (GetFileVersionInfo(VersionFilePath, 0, iVerInfoSize, pBuf)) {
			if (VerQueryValue(pBuf, TEXT("\\"), (void**)&pVsInfo, &iFileInfoSize)) {
				//主版本2.6.7.57
				//2
				int s_major_ver = (pVsInfo->dwFileVersionMS >> 16) & 0x0000FFFF;
				//6
				int s_minor_ver = pVsInfo->dwFileVersionMS & 0x0000FFFF;
				//7
				int s_build_num = (pVsInfo->dwFileVersionLS >> 16) & 0x0000FFFF;
				//57
				int s_revision_num = pVsInfo->dwFileVersionLS & 0x0000FFFF;

				//把版本变成字符串
				strstream wxVer;
				wxVer << s_major_ver << "." << s_minor_ver << "." << s_build_num << "." << s_revision_num;
				wxVer >> asVer;
			}
		}
		delete[] pBuf;
	}

	//版本匹配
	if (asVer == wxVersoin)
	{
		return TRUE;
	}

	//版本不匹配
	return FALSE;
}

//重新启动微信
VOID ReBootWeChat()
{
	//获取微信程序路径
	TCHAR szAppName[MAX_PATH];
	GetModuleFileName(NULL, szAppName, MAX_PATH);

	//启动新进程
	STARTUPINFO StartInfo;
	ZeroMemory(&StartInfo, sizeof(StartInfo));
	StartInfo.cb = sizeof(StartInfo);

	PROCESS_INFORMATION procStruct;
	ZeroMemory(&procStruct, sizeof(procStruct));
	StartInfo.cb = sizeof(STARTUPINFO);

	if (CreateProcess((LPCTSTR)szAppName, NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartInfo, &procStruct))
	{
		CloseHandle(procStruct.hProcess);
		CloseHandle(procStruct.hThread);
	}
	//终止当前进程
	TerminateProcess(GetCurrentProcess(), 0);
}

//Hook数据库信息
VOID HookWx()
{
	hookAddress = wxBaseAddress + 0x430FA3;
	jumpBackAddress = hookAddress + 6;

	BYTE JmpCode[6] = { 0 };
	JmpCode[0] = 0xE9;
	JmpCode[6 - 1] = 0x90;

	//新跳转指令中的数据=跳转的地址-原地址（HOOK的地址）-跳转指令的长度
	*(DWORD*)&JmpCode[1] = (DWORD)InlinkHookJump - hookAddress - 5;
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)hookAddress, JmpCode, 6, 0);
}

//InlineHook完成后，程序在Hook点跳转到这里执行。这里必须是裸函数
__declspec(naked) VOID InlinkHookJump()
{
	//补充代码
	__asm
	{
		//补充被覆盖的代码
		mov esi, dword ptr ss : [ebp - 0x14]
		add esp, 0x8

		//保存寄存器
		pushad

		//参数2，数据库句柄
		push[ebp - 0x14]
		//参数1，数据库路径地址，ASCII
		push[ebp - 0x28]
		//调用我们的处理函数
		call OutPutData
		add esp, 8

		//恢复寄存器
		popad

		//跳回去接着执行
		jmp jumpBackAddress
	}
}

//把内存中HOOK到的数据存储在链表中，重置定时器，5秒钟后激活定时器
VOID OutPutData(int dbAddress, int dbHandle)
{
	DbNameHandle db = { 0 };
	db.DBHandler = dbHandle;
	_snprintf_s(db.DBName, MAX_PATH, "%s", (char*)dbAddress);
	dbList.push_back(db);

	//定时器
	dwTimeId = SetTimer(NULL, 1, 5000, TimerProc);
}

//定时器回调函数，把内存中HOOK来的数据保存到一个文本文件中
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (dwTimeId == idEvent)
	{
		//关闭定时器
		KillTimer(NULL, 1);

		//把“数据库句柄-->数据库名”的链表保存到CombBox中
		HWND combo1 = GetDlgItem(hWinDlg, IDC_COMBO_DB);

		//删除全部内容
		while (SendMessage(combo1, CB_DELETESTRING, 0, 0) > 0) {}

		//添加内容
		for (auto& db : dbList)
		{
			SendMessageA(combo1, CB_ADDSTRING, NULL, (LPARAM)(db.DBName));
		}
	}
}

VOID UnInject()
{
	HMODULE hModule = NULL;

	//GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 会增加引用计数
	//因此，后面还需执行一次FreeLibrary
	//直接使用本函数（UnInject）地址来定位本模块
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPWSTR)&UnInject, &hModule);

	if (hModule != 0)
	{
		//减少一次引用计数
		FreeLibrary(hModule);
		//从内存中卸载
		FreeLibraryAndExitThread(hModule, 0);
	}
}

VOID RunSQL()
{
	DWORD selectedDbHander = 0;
	const char* sql = NULL;

	//清除文本框内容
	HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_LOG);
	SendMessageA(edit, WM_SETTEXT, NULL, NULL);

	//获取数据库句柄
	HWND combo1 = GetDlgItem(hWinDlg, IDC_COMBO_DB);
	int index = SendMessageA(combo1, CB_GETCURSEL, NULL, 0);
	char buf[MAX_PATH] = { 0 };	
	SendMessageA(combo1, CB_GETLBTEXT, index, (LPARAM)buf);

	//添加日志
	string text = "查询的数据库：\r\n";
	text.append(buf);
	text.append("\r\n");
	AddLog(text);

	//获取查询的数据库句柄
	string dbName(buf);
	char hexString[12] = { 0 };
	for (auto& db : dbList)
	{
		string dbNameInList(db.DBName);
		if (dbNameInList == dbName)
		{
			selectedDbHander = db.DBHandler;
			sprintf_s(hexString, "0x%08X", selectedDbHander);
			text = "数据库句柄:\r\n";
			text.append(hexString);
			text.append("\r\n");
			AddLog(text);

			break;
		}
	}

	//SQL语句,限制为MAX_PATH个字符，可做适当调整
	ZeroMemory(buf, MAX_PATH);
	//HWND sqlEdit = GetDlgItem(hWinDlg, IDC_EDIT_SQL);
	GetDlgItemTextA(hWinDlg, IDC_EDIT_SQL, buf, MAX_PATH);
	sql = buf;

	//调用sqlite3_exec函数查询数据库
	//char* errmsg = NULL;
	Sqlite3_exec sqlite3_exec = (Sqlite3_exec)(wxBaseAddress + 0x77F6C0);
	ZeroMemory(hexString, 12);
	sprintf_s(hexString, "0x%08X", (int)sqlite3_exec);
	text = "sqlite3_exec地址：\r\n";
	text.append(hexString);
	text.append("\r\n");
	AddLog(text);

	ZeroMemory(hexString, 12);
	sprintf_s(hexString, "0x%08p", MyCallback);
	text = "MyCallback地址：\r\n";
	text.append(hexString);
	text.append("\r\n");
	AddLog(text);

	DWORD i = sqlite3_exec(selectedDbHander, sql, MyCallback, NULL, NULL);
}

VOID AddLog(string text)
{
	HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_LOG);

	//获取当前文本框的字符数量
	int count = SendMessageA(edit, WM_GETTEXTLENGTH, NULL, NULL);

	//获取当前文本框的内容
	char* oldChars = new char[count + 1]{ 0 };
	SendMessageA(edit, WM_GETTEXT, (WPARAM)(count + 1), (LPARAM)oldChars);
	string oldText(oldChars);
	delete[] oldChars;

	//添加字符
	oldText.append(text);
	SendMessageA(edit, WM_SETTEXT, NULL, (LPARAM)(oldText.c_str()));
}

int __cdecl MyCallback(void* para, int nColumn, char** colValue, char** colName)
{
	string text = "--------------------------------------------------------------------------------------\r\n";
	AddLog(text);

	char* sOut = new char[1024 * 64];
	for (int i = 0; i < nColumn; i++)
	{
		ZeroMemory(sOut, 1024 * 64);
		sprintf_s(sOut, 1024, "%s :%s\n", *(colName + i), colValue[i]);
		text = string(sOut);
		text.append("\r\n");
		AddLog(text);	
	}
	delete[] sOut;
	return 0;
}