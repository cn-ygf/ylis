#include "luab.h"
#include <base/base.h>
#include <filesystem>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <tlhelp32.h>
#include <utils/utils.h>
#include <vector>
#include <windows.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Shell32.lib")

namespace fs = std::filesystem;

int lua_sleep(lua_State *L) {
	int n = luaL_checkinteger(L, 1);
	Sleep(n);
	return 0;
}

bool get_os_version(DWORD &dwMajorVersion, DWORD &dwMinorVersion,
					DWORD &dwBuildNumber);
bool CreateShortcutOnAllUsersDesktop(const std::wstring &shortcutName,
									 const std::wstring &targetPath,
									 const std::wstring &arguments = L"",
									 const std::wstring &iconPath = L"");
bool CreateShortcutInAllUsersStartMenu(
	const std::wstring &shortcutName, const std::wstring &targetPath,
	const std::wstring &arguments = L"", const std::wstring &iconPath = L"",
	const std::wstring &subfolder = L"" // 可选：开始菜单子文件夹
);

int lua_get_command_line(lua_State *L) {
	std::wstring sz_line = GetCommandLineW();
	lua_pushstring(L, utils::UTF16ToUTF8(sz_line).c_str());
	return 1;
}

int lua_get_args(lua_State *L) {
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv) {
		lua_newtable(L); // 返回空表
		return 1;
	}

	lua_newtable(L); // 创建一个 Lua 表

	for (int i = 0; i < argc; ++i) {
		std::string utf8 = utils::UTF16ToUTF8(argv[i]);
		lua_pushstring(L, utf8.c_str()); // 压入参数值
		lua_rawseti(L, -2, i + 1);		 // Lua 索引从 1 开始
	}

	LocalFree(argv);
	return 1; // 返回 1 个 Lua 值（table）
}

int lua_get_module_file_name(lua_State *L) {
	wchar_t szPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szPath, MAX_PATH);
	lua_pushstring(L, utils::UTF16ToUTF8(szPath).c_str());
	return 1;
}

int lua_get_local_app_data_dir(lua_State *L) {
	std::wstring sz_dir = nbase::win32::GetLocalAppDataDir();
	std::string sz_dir_a = nbase::UTF16ToUTF8(sz_dir);
	lua_pushstring(L, sz_dir_a.c_str());
	return 1;
}

int lua_get_roaming_app_data_dir(lua_State *L) {
	std::wstring sz_dir = nbase::win32::GetRoamingAppDataDir();
	std::string sz_dir_a = nbase::UTF16ToUTF8(sz_dir);
	lua_pushstring(L, sz_dir_a.c_str());
	return 1;
}

int lua_get_home_dir(lua_State *L) {
	wchar_t szPath[MAX_PATH] = {0};
	if (SHGetSpecialFolderPathW(NULL, szPath, CSIDL_PROFILE, FALSE)) {
		std::wstring sz = szPath;
		std::string sz_a = nbase::UTF16ToUTF8(sz);
		lua_pushstring(L, sz_a.c_str());
		return 1;
	} else {
		DWORD dwCode = GetLastError();
		return luaL_error(L, "call SHGetSpecialFolderPathW failed, dwCode: %d",
						  dwCode);
	}
}

int lua_is_admin(lua_State *L) {
	lua_pushboolean(L, nbase::win32::IsUserAdmin());
	return 1;
}

int lua_get_os_version(lua_State *L) {
	DWORD dwMajorVersion, dwMinorVersion, dwBuildNumber;
	if (!get_os_version(dwMajorVersion, dwMinorVersion, dwBuildNumber)) {
		return luaL_error(L, "get_os_version failed");
	}
	lua_pushnumber(L, dwMajorVersion);
	lua_pushnumber(L, dwMinorVersion);
	lua_pushnumber(L, dwBuildNumber);
	return 3;
}

typedef unsigned long NTSTATUS;
typedef NTSTATUS(NTAPI *FnRtlGetVersion)(
	PRTL_OSVERSIONINFOW lpVersionInformation);

bool get_os_version(DWORD &dwMajorVersion, DWORD &dwMinorVersion,
					DWORD &dwBuildNumber) {
	wchar_t lpszOSVersion[1024] = {0};
	HMODULE hModule = LoadLibrary(L"ntdll.dll");
	if (!hModule)
		return false;

	FnRtlGetVersion proc = NULL;
	proc = (FnRtlGetVersion)GetProcAddress(hModule, "RtlGetVersion");
	if (!proc)
		return false;

	OSVERSIONINFOW osInfo = {0};
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	NTSTATUS status = proc(&osInfo);
	FreeLibrary(hModule);
	if (status != 0)
		return false;

	dwMajorVersion = osInfo.dwMajorVersion;
	dwMinorVersion = osInfo.dwMinorVersion;
	dwBuildNumber = osInfo.dwBuildNumber;
	return true;
}

int lua_create_shortcut(lua_State *L) {
	const std::string shortcutName = luaL_checkstring(L, 1);
	const std::string targetPath = luaL_checkstring(L, 2);
	const std::string arguments = luaL_optstring(L, 3, "");
	const std::string iconPath = luaL_optstring(L, 4, "");

	if (!CreateShortcutOnAllUsersDesktop(
			utils::UTF8ToUTF16(shortcutName), utils::UTF8ToUTF16(targetPath),
			utils::UTF8ToUTF16(arguments), utils::UTF8ToUTF16(iconPath))) {
		return luaL_error(L, "CreateShortcutOnAllUsersDesktop failed");
	}
	return 0;
}

int lua_create_startmenu_shortcut(lua_State *L) {
	const std::string shortcutName = luaL_checkstring(L, 1);
	const std::string targetPath = luaL_checkstring(L, 2);
	const std::string arguments = luaL_optstring(L, 3, "");
	const std::string iconPath = luaL_optstring(L, 4, "");
	const std::string subfolder =
		luaL_optstring(L, 5, ""); // 新增参数：开始菜单子文件夹，默认空

	if (!CreateShortcutInAllUsersStartMenu(
			utils::UTF8ToUTF16(shortcutName), utils::UTF8ToUTF16(targetPath),
			utils::UTF8ToUTF16(arguments), utils::UTF8ToUTF16(iconPath),
			utils::UTF8ToUTF16(subfolder))) {
		return luaL_error(L, "CreateShortcutInAllUsersStartMenu failed");
	}
	return 0;
}

int lua_remove_shortcut(lua_State *L) {
	const std::string shortcutName = luaL_checkstring(L, 1);
	std::wstring shortcutNameW = utils::UTF8ToUTF16(shortcutName);
	wchar_t desktopPath[MAX_PATH];
	if (SHGetSpecialFolderPathW(NULL, desktopPath,
								CSIDL_COMMON_DESKTOPDIRECTORY, FALSE)) {
		std::wstring fullShortcutPath =
			std::wstring(desktopPath) + L"\\" + shortcutNameW + L".lnk";
		try {
			std::filesystem::remove(fullShortcutPath);
		} catch (...) {
		}
	}
	return 0;
}

int lua_remove_startmenu_shortcut(lua_State *L) {
	const std::string shortcutName = luaL_checkstring(L, 1);
	std::wstring shortcutNameW = utils::UTF8ToUTF16(shortcutName);
	wchar_t startMenuPath[MAX_PATH];

	if (SHGetSpecialFolderPathW(NULL, startMenuPath, CSIDL_COMMON_PROGRAMS,
								FALSE)) {
		std::wstring fullShortcutPath =
			std::wstring(startMenuPath) + L"\\" + shortcutNameW + L".lnk";
		try {
			std::filesystem::remove(fullShortcutPath);
		} catch (...) {
			// 可以选择返回错误或者忽略
		}
	}
	return 0;
}

bool CreateShortcutOnAllUsersDesktop(const std::wstring &shortcutName,
									 const std::wstring &targetPath,
									 const std::wstring &arguments,
									 const std::wstring &iconPath) {
	HRESULT hr;
	IShellLinkW *pShellLink = nullptr;

	hr = CoInitialize(NULL);
	if (FAILED(hr))
		return false;

	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
						  IID_IShellLinkW, (LPVOID *)&pShellLink);
	if (SUCCEEDED(hr)) {
		pShellLink->SetPath(targetPath.c_str());
		if (!arguments.empty())
			pShellLink->SetArguments(arguments.c_str());
		if (!iconPath.empty())
			pShellLink->SetIconLocation(iconPath.c_str(), 0);

		IPersistFile *pPersistFile;
		hr = pShellLink->QueryInterface(IID_IPersistFile,
										(LPVOID *)&pPersistFile);
		if (SUCCEEDED(hr)) {
			wchar_t desktopPath[MAX_PATH];
			if (SHGetSpecialFolderPathW(NULL, desktopPath,
										CSIDL_COMMON_DESKTOPDIRECTORY, FALSE)) {
				std::wstring fullShortcutPath =
					std::wstring(desktopPath) + L"\\" + shortcutName + L".lnk";
				hr = pPersistFile->Save(fullShortcutPath.c_str(), TRUE);
			}
			pPersistFile->Release();
		}
		pShellLink->Release();
	}

	CoUninitialize();
	return SUCCEEDED(hr);
}

bool CreateShortcutInAllUsersStartMenu(
	const std::wstring &shortcutName, const std::wstring &targetPath,
	const std::wstring &arguments, const std::wstring &iconPath,
	const std::wstring &subfolder // 可选：开始菜单子文件夹
) {
	HRESULT hr;
	IShellLinkW *pShellLink = nullptr;

	hr = CoInitialize(NULL);
	if (FAILED(hr))
		return false;

	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
						  IID_IShellLinkW, (LPVOID *)&pShellLink);
	if (SUCCEEDED(hr)) {
		pShellLink->SetPath(targetPath.c_str());
		if (!arguments.empty())
			pShellLink->SetArguments(arguments.c_str());
		if (!iconPath.empty())
			pShellLink->SetIconLocation(iconPath.c_str(), 0);

		IPersistFile *pPersistFile;
		hr = pShellLink->QueryInterface(IID_IPersistFile,
										(LPVOID *)&pPersistFile);
		if (SUCCEEDED(hr)) {
			wchar_t startMenuPath[MAX_PATH];
			if (SHGetSpecialFolderPathW(NULL, startMenuPath,
										CSIDL_COMMON_PROGRAMS, FALSE)) {
				std::wstring fullDir = startMenuPath;
				if (!subfolder.empty()) {
					fullDir += L"\\" + subfolder;
					CreateDirectoryW(fullDir.c_str(), NULL); // 确保子目录存在
				}

				std::wstring fullPath =
					fullDir + L"\\" + shortcutName + L".lnk";
				hr = pPersistFile->Save(fullPath.c_str(), TRUE);
			}
			pPersistFile->Release();
		}
		pShellLink->Release();
	}

	CoUninitialize();
	return SUCCEEDED(hr);
}

int lua_kill_process(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);
	uint32_t timeout =
		static_cast<uint32_t>(luaL_optinteger(L, 2, 10000)); // 默认10秒

	std::string err;
	if (utils::kill_process_by_name(name, timeout, err)) {
		return 0;
	}
	return luaL_error(L, "kill_process failed: %s", err.c_str());
}

// 获取进程列表
// 返回table
int lua_get_process(lua_State *L) {
	// 创建一个进程快照
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		DWORD dwCode = GetLastError();
		return luaL_error(L, "Failed to create process snapshot, code: %d", dwCode);
	}

	// 用于遍历进程
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnapshot, &pe32)) {
		CloseHandle(hSnapshot);
		DWORD dwCode = GetLastError();
		return luaL_error(L, "call Process32First, code: %d", dwCode);
	}

	// 创建返回的 Lua table
	lua_newtable(L);
	int index = 1;

	do {
		lua_newtable(L); // 新建一个子table

		lua_pushstring(L, "pid");
		lua_pushinteger(L, pe32.th32ProcessID);
		lua_settable(L, -3);

		lua_pushstring(L, "ppid");
		lua_pushinteger(L, pe32.th32ParentProcessID);
		lua_settable(L, -3);

		std::string exe = utils::UTF16ToUTF8(pe32.szExeFile);
		lua_pushstring(L, "exe");
		lua_pushstring(L, exe.c_str());
		lua_settable(L, -3);

		lua_rawseti(L, -2, index++);
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return 1; // 返回 table
}

int lua_get_physical_mac_address_list(lua_State *L) {
	lua_newtable(L);

	std::vector<std::string> mac_addresses;
	BOOL bRet = utils::GetPhysicalMacAddressList(mac_addresses);
	if (!bRet) {
		return 1;
	}
	size_t index = 1;
	for (auto &item : mac_addresses) {
		lua_pushinteger(L, index);
		lua_pushstring(L, item.c_str());
		lua_settable(L, -3);
		index += 1;
	}
	return 1;
}

int lua_get_physical_ip_address_list(lua_State *L) {
	lua_newtable(L);

	std::vector<std::string> ip_addresses;
	BOOL bRet = utils::GetPhysicalIpAddressList(ip_addresses);
	if (!bRet) {
		return 1;
	}
	size_t index = 1;
	for (auto &item : ip_addresses) {
		lua_pushinteger(L, index);
		lua_pushstring(L, item.c_str());
		lua_settable(L, -3);
		index += 1;
	}
	return 1;
}

// 判断是否已经有命名mutex
// 用来判断安装包多次运行
// 返回true代表已经运行
int lua_check_mutex(lua_State *L) {
	std::string mutex_name = luaL_checkstring(L, 1);
	std::wstring mutex_name_w = utils::UTF8ToUTF16(mutex_name);
	HANDLE hMutex = CreateMutex(NULL, FALSE, mutex_name_w.c_str());
	if (hMutex == NULL) {
		DWORD dwCode = GetLastError();
		return luaL_error(L, "CreateMutex failed, dwCode: %d", dwCode);
	}
	DWORD dwErrorCode = GetLastError();
	lua_pushboolean(L, dwErrorCode == ERROR_ALREADY_EXISTS);
	return 1;
}

// 结束进程
int lua_exit_process(lua_State *L) {
	int exit_code = luaL_optinteger(L, 1, 0);
	ExitProcess(exit_code);
	return 0;
}