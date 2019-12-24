#include "pch.h"
#include "resource.h"
#include "shellapi.h"
#include <string>
#include <strstream>
#include <list>
#include <CommCtrl.h>
#include <commdlg.h>
#pragma comment(lib, "Version.lib")
using namespace std;

VOID ShowDemoUI(HMODULE hModule);
INT_PTR CALLBACK DialogProc(_In_ HWND   hwndDlg, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
BOOL IsWxVersionValid();
VOID UnInject();
VOID HookWx();
VOID InlinkHookJump();
VOID OutPutData(int dbAddress, int dbHandle);
VOID ReBootWeChat();
VOID RunBackUpDb();
VOID RefreshComboBox();
VOID BackUpDB();
VOID AddLog(string text);

void UpdateProgress(int pos, int all);
wstring StringToWString(string str);
string WStringToString(const wstring& wstr);
int MakeBackupDbInMemo();
void BackupSQLiteDB();
void XProgress(int a, int b);

typedef int(__cdecl* BackupDb)(
	DWORD,               /* Database to back up */
	DWORD,      /* Name of file to back up to */
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	void(*xProgress)(int, int)  /* Progress function to invoke */
	);


DWORD wxBaseAddress = 0;
HWND hWinDlg;
const string wxVersoin = "2.6.8.65";
BOOL isWxHooked = FALSE;
DWORD hookAddress = 0;
DWORD overWritedCallAdd = 0;
DWORD jumpBackAddress = 0;
DWORD dwTimeId = 0;
//重新取当前的数据库句柄
DWORD selectedDbHander = 0;
string backupFileName2;
DWORD lpAddressBackupDB = 0;


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

void UpdateProgress(int pos, int all)
{
	HWND hProgress = GetDlgItem(hWinDlg, IDC_PROGRESS);
	//设置进度条范围
	//SendMessageA(hProgress, PBM_SETRANGE, NULL, MAKELPARAM(0, 100));
	//设置进度条进度
	SendMessageA(hProgress, PBM_SETPOS, (all - pos) * 100 / all, 0);
	UpdateWindow(hProgress);

	//设置文本内容
	HWND hStatic = GetDlgItem(hWinDlg, IDC_STATIC_PROGRESS);
	char dataPercent[7] = { 0 };
	sprintf_s(dataPercent, 7, "%.2f", (100.0f * (all - pos)) / all);
	string posStr(dataPercent);
	posStr.append("%");
	SendMessageA(hStatic, WM_SETTEXT, NULL, (LPARAM)(posStr.c_str()));
	UpdateWindow(hStatic);
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

		//在内存中创建函数
		lpAddressBackupDB = MakeBackupDbInMemo();

		HWND hProgress = GetDlgItem(hWinDlg, IDC_PROGRESS);
		//设置进度条范围
		SendMessageA(hProgress, PBM_SETRANGE, NULL, MAKELPARAM(0, 100));
		//设置进度条进度
		SendMessageA(hProgress, PBM_SETPOS, 0, 0);
		UpdateWindow(hProgress);

		HWND hStatic = GetDlgItem(hWinDlg, IDC_STATIC_PROGRESS);
		//设置文本内容
		SendMessageA(hStatic, WM_SETTEXT, NULL, (LPARAM)("00.00%"));
		UpdateWindow(hStatic);

		break;
	}
	case WM_CLOSE:
	{
		//恢复HOOK掉的代码...省略unhook
		//释放在内存中创建的函数(lpAddressBackupDB)...省略  
		//从内存中卸载本dll
		UnInject();
		EndDialog(hwndDlg, 0);
		break;
	}
	case WM_COMMAND:
	{
		//执行选择
		if (LOWORD(wParam) == IDC_COMBO_DB)
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				//获取数据库句柄
				int index = SendMessageA((HWND)lParam, CB_GETCURSEL, NULL, 0);
				char buf[MAX_PATH] = { 0 };
				SendMessageA((HWND)lParam, CB_GETLBTEXT, index, (LPARAM)buf);

				//清空文本框
				HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_LOG);
				SendMessageA(edit, WM_SETTEXT, NULL, NULL);

				//添加日志
				string text = "在线备份的数据库：\r\n";
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
						sprintf_s(hexString, "0x%08X", db.DBHandler);
						text = "数据库句柄:\r\n";
						text.append(hexString);
						text.append("\r\n");
						AddLog(text);

						break;
					}
				}
			}
			break;
		}

		//刷新ComboBox
		if (wParam == IDC_BUTTON_REFRESH)
		{
			RefreshComboBox();
			break;
		}

		//执行备份
		if (wParam == IDC_BUTTON_SQLRUN)
		{
			BackUpDB();
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

	//使能 刷新按钮
	HWND hRefresh = GetDlgItem(hWinDlg, IDC_BUTTON_REFRESH);
	EnableWindow(hRefresh, TRUE);

	AddLog(string(db.DBName));
	AddLog(string("\r\n"));
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

VOID RunBackUpDb()
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
	string text = "在线备份的数据库：\r\n";
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
			break;
		}
	}

	//SQL语句,限制为MAX_PATH个字符，可做适当调整
	BackUpDB();
}


string WStringToString(const wstring& wstr)
{
	string str;
	int nLen = (int)wstr.length();
	str.resize(nLen, ' ');
	int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);
	if (nResult == 0)
	{
		return "";
	}
	return str;
}


wstring StringToWString(string str)
{
	wstring result;
	//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//多字节编码转换成宽字节编码  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';             //添加字符串结尾  
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
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

VOID RefreshComboBox()
{
	//禁止 刷新按钮
	HWND hRefresh = GetDlgItem(hWinDlg, IDC_BUTTON_REFRESH);
	EnableWindow(hRefresh, FALSE);

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

VOID BackUpDB()
{
	//清除文本框内容
	HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_LOG);
	SendMessageA(edit, WM_SETTEXT, NULL, NULL);

	//获取数据库句柄
	HWND combo1 = GetDlgItem(hWinDlg, IDC_COMBO_DB);
	int index = SendMessageA(combo1, CB_GETCURSEL, NULL, 0);
	char buf[MAX_PATH] = { 0 };
	SendMessageA(combo1, CB_GETLBTEXT, index, (LPARAM)buf);

	if (string(buf) == "")
	{
		AddLog("请先选择要在线备份的数据库！\r\n");
		return;
	}

	//添加日志
	string text = "数据库：\r\n";
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
			break;
		}
	}

	if (selectedDbHander == 0)
	{
		AddLog("请先选择要在线备份的数据库！\r\n");
		return;
	}

	//获取默认文件名
	TCHAR driver[_MAX_DRIVE] = { 0 };
	TCHAR dir[_MAX_DIR] = { 0 };
	TCHAR fname[_MAX_FNAME] = { 0 };
	TCHAR ext[_MAX_EXT] = { 0 };
	TCHAR szFile[MAX_PATH] = { 0 };
	_wsplitpath_s(StringToWString(dbName).c_str(), driver, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
	wstring fileName(fname);
	fileName.append(ext);
	wmemcpy(szFile, fileName.c_str(), fileName.length());

	//程序当前目录
	TCHAR szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);

	// 公共对话框结构。   
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetForegroundWindow();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"SQL(*.db)\0*.db\0All(*.*)\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = szCurDir;
	ofn.lpstrDefExt = L"db";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_CREATEPROMPT | OFN_EXTENSIONDIFFERENT;

	// 显示打开选择文件对话框。
	if (GetSaveFileName(&ofn))
	{
		wstring dumpFile(szFile);
		backupFileName2 = WStringToString(dumpFile);
		string text = "备份的数据库:\r\n";
		text.append(backupFileName2);
		text.append("\r\n");
		AddLog(text);

		//以线程的方式启动
		//这样界面不会卡死
		HANDLE hANDLE = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BackupSQLiteDB, NULL, NULL, 0);
		if (hANDLE != 0)
		{
			CloseHandle(hANDLE);
		}
	}
}

void BackupSQLiteDB()
{
	//text:107A5180
	DWORD address_sqlite3_open = wxBaseAddress + 0x7A5180;
	//.text:1074EA30 
	DWORD address_sqlite3_backup_init = wxBaseAddress + 0x74EA30;
	//text:1074ED70 
	DWORD address_sqlite3_backup_step = wxBaseAddress + 0x74ED70;
	//.text:107A57F0
	DWORD address_sqlite3_sleep = wxBaseAddress + 0x07A57F0;
	//.text:1074F4C0 
	DWORD address_sqlite3_backup_finish = wxBaseAddress + 0x074F4C0;
	//.text:107A3000 
	DWORD address_sqlite3_close = wxBaseAddress + 0x7A3000;
	//text:1074F5C0
	DWORD address_sqlite3_backup_remaining = wxBaseAddress + 0x74F5C0;
	//.text:1074F5D0
	DWORD address_sqlite3_backup_pagecount = wxBaseAddress + 0x74F5D0;
	//text:107A4250
	DWORD address_sqlite3_errcode = wxBaseAddress + 0x7A4250;

	const char* backupFile = backupFileName2.c_str();
	const char* myMain = "main";

	BackupDb p_BackupDb = (BackupDb)lpAddressBackupDB;
	p_BackupDb(
		selectedDbHander,
		(DWORD)backupFile,
		(DWORD)myMain,
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

	string text = "备份完成！\r\n";
	AddLog(text);

	//询问是否查看文件
	int result = MessageBoxA(NULL, "备份完成，是否查看？", "完成", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_APPLMODAL | MB_TOPMOST);
	if (result == IDYES)
	{
		//获取默认文件名
		TCHAR driver[_MAX_DRIVE] = { 0 };
		TCHAR dir[_MAX_DIR] = { 0 };
		TCHAR fname[_MAX_FNAME] = { 0 };
		TCHAR ext[_MAX_EXT] = { 0 };
		TCHAR szFile[MAX_PATH] = { 0 };
		_wsplitpath_s(StringToWString(backupFileName2).c_str(), driver, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
		wstring filePath(driver);
		filePath.append(dir);

		ShellExecute(NULL, L"explore", filePath.c_str(), NULL, NULL, SW_SHOW);
	}
}

int MakeBackupDbInMemo()
{
	string data = "55 8B EC 83 EC 30 8B 45 14 89 45 F0 8B 4D 18 89 4D EC 8B 55 1C 89 55 E8 8B 45 20 89 45 E0 8B 4D24 89 4D E4 8B 55 28 89 55 DC 8B 45 2C 89 45 D8 8B 4D 30 89 4D D4 8B 55 34 89 55 D0 8D 45 F4 508B 4D 0C 51 FF 55 F0 83 C4 08 89 45 FC 83 7D FC 00 0F 85 8D 00 00 00 8B 55 10 52 8B 45 08 50 8B4D 10 51 8B 55 F4 52 FF 55 EC 83 C4 10 89 45 F8 83 7D F8 00 74 61 6A 05 8B 45 F8 50 FF 55 E8 83C4 08 89 45 FC 8B 4D F8 51 FF 55 E4 83 C4 04 50 8B 55 F8 52 FF 55 E0 83 C4 04 50 FF 55 38 83 C408 83 7D FC 00 74 0C 83 7D FC 05 74 06 83 7D FC 06 75 08 6A 32 FF 55 DC 83 C4 04 83 7D FC 00 74B5 83 7D FC 05 74 AF 83 7D FC 06 74 A9 8B 45 F8 50 FF 55 D8 83 C4 04 8B 4D F4 51 FF 55 D4 83 C404 89 45 FC 8B 55 F4 52 FF 55 D0 83 C4 04 8B 45 FC 8B E5 5D C3";
	string space = " ";
	int pos = data.find(space);
	while (pos != -1)
	{
		data.replace(pos, space.length(), "");
		pos = data.find(space);
	}

	int codeLength = data.length() / 2;

	unsigned char* code = new unsigned char[codeLength] {0};
	for (int i = 0; i < codeLength; i++)
	{
		string byteCode = data.substr(i * 2, 2);
		if (byteCode[0] >= '0' && byteCode[0] <= '9')
		{
			code[i] = (byteCode[0] - '0') * 16;
		}
		else
		{
			code[i] = (unsigned char)((byteCode[0] - 'A' + 10) * 16);
		}

		if (byteCode[1] >= '0' && byteCode[1] <= '9')
		{
			code[i] += (byteCode[1] - '0');
		}
		else
		{
			code[i] += (unsigned char)(byteCode[1] - 'A' + 10);
		}
	}

	//申请内存控件，在内存中构造函数
	LPVOID lpAddressBackupDB = VirtualAlloc(NULL, codeLength, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (lpAddressBackupDB == NULL)
	{
		printf("VirtualAlloc error: %d\n", GetLastError());
		return 1;
	}
	CopyMemory(lpAddressBackupDB, code, codeLength);

	return (int)lpAddressBackupDB;
}

//回调函数，更新进度
void XProgress(int a, int b)
{
	UpdateProgress(a, b);
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
