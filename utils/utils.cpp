#include "utils.h"
#include <atlbase.h>
#include <base/base.h>
#include <chrono>
#include <cwctype>
#include <fstream>
#include <hv/md5.h>
#include <iomanip>
#include <sstream>
#include <thread>
#include <tlhelp32.h>
#include <vector>

#include <WtsApi32.h>
#include <iphlpapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <windows.h>
#include <winsock2.h> // winsock2 必须在 windows.h 之前
#include <ws2tcpip.h>

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace utils {

std::string CalculateFileMD5(const std::wstring &filename) {

	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
		return "";
	HV_MD5_CTX ctx;
	HV_MD5Init(&ctx);

	const size_t buffer_size = 8192;
	uint8_t buffer[buffer_size];
	while (file.good()) {
		file.read((char *)buffer, buffer_size);
		std::streamsize bytes_read = file.gcount();
		if (bytes_read > 0) {
			HV_MD5Update(&ctx, buffer, bytes_read);
		}
	}
	unsigned char digest[16];
	HV_MD5Final(&ctx, digest);

	std::ostringstream oss;
	for (int i = 0; i < 16; ++i) {
		oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
	}
	return oss.str();
}

bool CalculateFileMD5(const std::wstring &filename, unsigned char digest[16]) {

	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
		return false;
	HV_MD5_CTX ctx;
	HV_MD5Init(&ctx);

	const size_t buffer_size = 8192;
	uint8_t buffer[buffer_size];
	while (file.good()) {
		file.read((char *)buffer, buffer_size);
		std::streamsize bytes_read = file.gcount();
		if (bytes_read > 0) {
			HV_MD5Update(&ctx, buffer, bytes_read);
		}
	}

	HV_MD5Final(&ctx, digest);
	return true;
}

std::string CalculateMD5(const uint8_t *data, unsigned long size) {
	char buffer[33] = {0};
	hv_md5_hex((uint8_t *)data, size, buffer, 33);
	std::string result = buffer;
	return result;
}

std::string MakeAbsolutePath(const std::string &path,
							 const std::string &base_path) {
	if (!PathIsRelativeA(path.c_str())) {
		// 已经是绝对路径，直接返回
		return path;
	}
	std::string absPath = base_path;
	// 确保当前目录末尾有 '\\'
	if (absPath.back() != '\\' && absPath.back() != '/') {
		absPath += '\\';
	}

	absPath += path;
	return absPath;
}

std::string MakeAbsolutePath(const std::string &path) {
	if (!PathIsRelativeA(path.c_str())) {
		// 已经是绝对路径，直接返回
		return path;
	}

	// 获取当前目录
	char buffer[MAX_PATH];
	DWORD len = GetCurrentDirectoryA(MAX_PATH, buffer);
	if (len == 0 || len > MAX_PATH) {
		// 获取失败
		return path;
	}

	std::string absPath = buffer;

	// 确保当前目录末尾有 '\\'
	if (absPath.back() != '\\' && absPath.back() != '/') {
		absPath += '\\';
	}

	absPath += path;
	return absPath;
}
bool IsDriveLetterAvailable(wchar_t driveLetter) {
	driveLetter = std::towupper(driveLetter);
	if (driveLetter < L'A' || driveLetter > L'Z')
		return false;

	DWORD drives = GetLogicalDrives();
	return (drives & (1 << (driveLetter - L'A'))) != 0;
}

bool IsPathSyntaxValid(const std::wstring &path) {
	// 1. 空路径非法
	if (path.empty())
		return false;

	// 2. 路径长度不能太长
	if (path.length() >= MAX_PATH)
		return false;

	// 3. 必须是绝对路径，形如 "C:\Something"
	if (path.length() < 3 || path[1] != L':' || path[2] != L'\\')
		return false;

	// 4. 检查非法字符（允许第2个字符是冒号）
	const std::wstring invalidChars = L"<>\"|?*"; // 移除冒号（:）

	for (size_t i = 0; i < path.length(); ++i) {
		wchar_t ch = path[i];

		// 除了盘符后的位置，其它地方不允许冒号
		if (ch == L':' && i != 1) {
			return false;
		}

		if (invalidChars.find(ch) != std::wstring::npos) {
			return false;
		}
	}

	return IsDriveLetterAvailable(path.at(0));
}

std::string LocalToUTF8(const std::string &local) {
	std::wstring a = LocalToUTF16(local);
	return UTF16ToUTF8(a);
}

std::string UTF8ToLocal(const std::string &utf8) {
	std::wstring a = UTF8ToUTF16(utf8);
	return UTF16ToLocal(a);
}

std::wstring UTF8ToUTF16(const std::string &utf8) {
	if (utf8.empty()) {
		return std::wstring();
	}

	int requiredSize = MultiByteToWideChar(
		CP_UTF8,					   // 使用 UTF-8 编码
		0,							   // 无特殊标志
		utf8.data(),				   // 输入字符串指针
		static_cast<int>(utf8.size()), // 输入字符串长度（字节数）
		nullptr,					   // 不获取输出缓冲
		0							   // 获取所需缓冲区大小
	);

	if (requiredSize == 0) {
		return std::wstring();
	}

	std::wstring utf16(requiredSize, L'\0');
	int convertedSize = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
											static_cast<int>(utf8.size()),
											&utf16[0], // 输出缓冲区
											requiredSize);

	if (convertedSize == 0) {
		return std::wstring();
	}

	return utf16;
}

std::string UTF16ToUTF8(const std::wstring &utf16) {
	if (utf16.empty()) {
		return std::string();
	}

	int requiredSize = WideCharToMultiByte(
		CP_UTF8,						// 使用 UTF-8 编码
		0,								// 无特殊标志
		utf16.data(),					// 输入字符串指针
		static_cast<int>(utf16.size()), // 输入字符串长度（字符数）
		nullptr,						// 不获取输出缓冲
		0,								// 获取所需缓冲区大小
		nullptr,						// 默认字符（无需设置）
		nullptr							// 默认字符使用标记（无需设置）
	);

	if (requiredSize == 0) {
		return std::string();
	}

	std::string utf8(requiredSize, '\0');
	int convertedSize = WideCharToMultiByte(CP_UTF8, 0, utf16.data(),
											static_cast<int>(utf16.size()),
											&utf8[0], // 输出缓冲区
											requiredSize, nullptr, nullptr);

	if (convertedSize == 0) {
		return std::string();
	}

	return utf8;
}

std::string UTF16ToLocal(const std::wstring &wstr) {
	if (wstr.empty()) {
		return "";
	}

	// 计算所需的多字节长度
	int mb_len = WideCharToMultiByte(CP_ACP,	   // 使用本地 ANSI 代码页
									 0,			   // 无特殊标志
									 wstr.c_str(), // 源宽字符字符串
									 static_cast<int>(wstr.size()), // 字符数
									 nullptr,		  // 不接收输出缓冲区
									 0,				  // 查询所需字节数
									 nullptr, nullptr // 不使用默认字符替代
	);

	if (mb_len == 0) {
		return "";
	}

	std::string result;
	result.resize(mb_len);

	int result_len = WideCharToMultiByte(
		CP_ACP, 0, wstr.c_str(), static_cast<int>(wstr.size()), result.data(),
		mb_len, nullptr, nullptr);

	if (result_len == 0) {
		return "";
	}

	return result;
}

std::wstring LocalToUTF16(const std::string &str) {
	if (str.empty()) {
		return L"";
	}

	// 计算所需缓冲区长度
	int wide_len = MultiByteToWideChar(
		CP_ACP,						  // 使用当前ANSI代码页
		0,							  // 无特殊标志
		str.c_str(),				  // 源字符串指针
		static_cast<int>(str.size()), // 字符串长度（字节数）
		nullptr,					  // 不接收输出缓冲区
		0							  // 询问所需缓冲区长度
	);

	if (wide_len == 0) {
		return L"";
	}

	std::wstring wstr;
	wstr.resize(wide_len);

	int result = MultiByteToWideChar(CP_ACP, 0, str.c_str(),
									 static_cast<int>(str.size()),
									 (LPWSTR)wstr.data(), wide_len);

	if (result == 0) {
		return L"";
	}

	return wstr;
}

bool create_process(const std::string &command) {
	std::wstring command_w = nbase::UTF8ToUTF16(command);

	// 设置启动信息
	STARTUPINFO si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi = {0};

	// 启动进程
	if (!CreateProcess(NULL, const_cast<LPWSTR>(command_w.c_str()), NULL, NULL,
					   FALSE, 0, NULL, NULL, &si, &pi)) {
		return false;
	}
	// 关闭句柄
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}

bool create_process_wait(const std::string &command) {
	std::wstring command_w = nbase::UTF8ToUTF16(command);

	// 设置启动信息
	STARTUPINFO si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi = {0};

	// 启动进程
	if (!CreateProcess(NULL, const_cast<LPWSTR>(command_w.c_str()), NULL, NULL,
					   FALSE, 0, NULL, NULL, &si, &pi)) {
		return false;
	}
	// 关闭句柄
	CloseHandle(pi.hThread);
	// 等待进程结束
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	return true;
}

bool exec_command(const std::string &command, std::string &stdOut,
				  std::string &stdErr) {
	stdOut.clear();
	stdErr.clear();

	std::wstring command_w = nbase::UTF8ToUTF16(command);

	HANDLE hStdOutRead = NULL, hStdOutWrite = NULL;
	HANDLE hStdErrRead = NULL, hStdErrWrite = NULL;

	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

	// 创建标准输出管道
	if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
		stdErr = "Failed to create stdout pipe.";
		return false;
	}
	if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
		stdErr = "Failed to set stdout pipe handle information.";
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		return false;
	}

	// 创建标准错误管道
	if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0)) {
		stdErr = "Failed to create stderr pipe.";
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		return false;
	}
	if (!SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0)) {
		stdErr = "Failed to set stderr pipe handle information.";
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		CloseHandle(hStdErrRead);
		CloseHandle(hStdErrWrite);
		return false;
	}

	// 设置启动信息
	STARTUPINFO si = {sizeof(STARTUPINFO)};
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = hStdOutWrite;
	si.hStdError = hStdErrWrite;

	PROCESS_INFORMATION pi = {0};

	// 启动进程
	if (!CreateProcess(NULL, const_cast<LPWSTR>(command_w.c_str()), NULL, NULL,
					   TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		stdErr = "Failed to create process. Error: " +
				 std::to_string(GetLastError());
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		CloseHandle(hStdErrRead);
		CloseHandle(hStdErrWrite);
		return false;
	}

	// 关闭不需要的写句柄
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);

	// 读取标准输出和标准错误
	stdOut.clear();
	stdErr.clear();
	CHAR buffer[4096];
	DWORD bytesRead;

	// 读取标准输出
	while (
		ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) &&
		bytesRead > 0) {
		buffer[bytesRead] = '\0'; // 确保以 NULL 结尾
		stdOut += utils::LocalToUTF8(std::string(buffer, buffer + bytesRead));
	}

	// 读取标准错误
	while (
		ReadFile(hStdErrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) &&
		bytesRead > 0) {
		buffer[bytesRead] = '\0'; // 确保以 NULL 结尾
		stdErr += utils::LocalToUTF8(std::string(buffer, buffer + bytesRead));
	}

	// 等待进程结束
	WaitForSingleObject(pi.hProcess, INFINITE);

	// 关闭句柄
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hStdOutRead);
	CloseHandle(hStdErrRead);

	return stdErr.empty(); // 如果标准错误为空，则返回 TRUE，否则返回 FALSE
}

/*
 * 判断当前进程是否为System权限
 */
bool IsRunningAsSystem() {
	BOOL bIsSystem = FALSE;
	HANDLE hToken = NULL;
	DWORD dwSize = 0;
	PTOKEN_USER pTokenUser = NULL;
	PSID pSystemSid = NULL;

	// 1. 打开当前进程的令牌
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		return FALSE;
	}

	// 2. 获取令牌用户信息所需缓冲区大小
	if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize) &&
		(GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
		CloseHandle(hToken);
		return FALSE;
	}

	// 分配缓冲区
	pTokenUser = (PTOKEN_USER)malloc(dwSize);
	if (!pTokenUser) {
		CloseHandle(hToken);
		return FALSE;
	}

	// 获取令牌用户信息
	if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
		free(pTokenUser);
		CloseHandle(hToken);
		return FALSE;
	}

	// 3. 创建SYSTEM账户的SID
	DWORD cbSid = SECURITY_MAX_SID_SIZE;
	pSystemSid = LocalAlloc(LPTR, cbSid);
	if (!pSystemSid) {
		free(pTokenUser);
		CloseHandle(hToken);
		return FALSE;
	}

	if (!CreateWellKnownSid(WinLocalSystemSid, NULL, pSystemSid, &cbSid)) {
		LocalFree(pSystemSid);
		free(pTokenUser);
		CloseHandle(hToken);
		return FALSE;
	}

	// 4. 比较SID是否相同
	if (EqualSid(pTokenUser->User.Sid, pSystemSid)) {
		bIsSystem = TRUE;
	}

	// 清理资源
	LocalFree(pSystemSid);
	free(pTokenUser);
	CloseHandle(hToken);

	return bIsSystem;
}

BOOL SetRegistryMultiStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
								 LPCTSTR pszValueName, LPCTSTR pszValue) {
	HKEY hKey;
	LONG r = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, KEY_SET_VALUE, &hKey);
	if (r != ERROR_SUCCESS) {
		r = RegCreateKeyEx(hKeyParent, lpszKeyName, 0, NULL, 0, KEY_SET_VALUE,
						   NULL, &hKey, NULL);
		if (r != ERROR_SUCCESS)
			return FALSE;
	}

	// 多字符串是 REG_MULTI_SZ，内容格式： str1\0str2\0...\0\0
	DWORD cbData = 0;
	for (LPCTSTR p = pszValue; *p != '\0' || *(p + 1) != '\0'; ++p) {
		cbData += sizeof(TCHAR);
	}
	cbData += 2 * sizeof(TCHAR); // 最后两个 \0

	r = RegSetValueEx(hKey, pszValueName, 0, REG_MULTI_SZ,
					  (const BYTE *)pszValue, cbData);
	RegCloseKey(hKey);
	return (r == ERROR_SUCCESS);
}

BOOL SetRegistryDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
						   LPCTSTR pszValueName, DWORD dwValue) {
	HKEY hKey;
	LONG r = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, KEY_SET_VALUE, &hKey);
	if (r != ERROR_SUCCESS) {
		r = RegCreateKeyEx(hKeyParent, lpszKeyName, 0, NULL, 0, KEY_SET_VALUE,
						   NULL, &hKey, NULL);
		if (r != ERROR_SUCCESS)
			return FALSE;
	}

	r = RegSetValueEx(hKey, pszValueName, 0, REG_DWORD, (const BYTE *)&dwValue,
					  sizeof(DWORD));
	RegCloseKey(hKey);
	return (r == ERROR_SUCCESS);
}

BOOL SetRegistryStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
							LPCTSTR pszValueName, LPCTSTR pszValue) {
	HKEY hKey;
	LONG r = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, KEY_SET_VALUE, &hKey);
	if (r != ERROR_SUCCESS) {
		r = RegCreateKeyEx(hKeyParent, lpszKeyName, 0, NULL, 0, KEY_SET_VALUE,
						   NULL, &hKey, NULL);
		if (r != ERROR_SUCCESS)
			return FALSE;
	}

	DWORD cbData = (lstrlen(pszValue) + 1) * sizeof(TCHAR);
	r = RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (const BYTE *)pszValue,
					  cbData);
	RegCloseKey(hKey);
	return (r == ERROR_SUCCESS);
}

BOOL SetRegistryStringValueEx(HKEY hKeyParent, LPCTSTR lpszKeyName,
							  LPCTSTR pszValueName, LPCTSTR pszValue) {
	HKEY hKey;
	LONG r = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, KEY_SET_VALUE, &hKey);
	if (r != ERROR_SUCCESS) {
		r = RegCreateKeyEx(hKeyParent, lpszKeyName, 0, NULL, 0, KEY_SET_VALUE,
						   NULL, &hKey, NULL);
		if (r != ERROR_SUCCESS)
			return FALSE;
	}

	DWORD cbData = (lstrlen(pszValue) + 1) * sizeof(TCHAR);
	r = RegSetValueEx(hKey, pszValueName, 0, REG_EXPAND_SZ,
					  (const BYTE *)pszValue, cbData);
	RegCloseKey(hKey);
	return (r == ERROR_SUCCESS);
}

BOOL DeleteRegistryKey(HKEY hKeyParent, LPCTSTR lpszKeyName) {
	return (RegDeleteTree(hKeyParent, lpszKeyName) == ERROR_SUCCESS);
}

BOOL DeleteRegistryValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
						 LPCTSTR lpszValueName) {
	HKEY hKey;
	LONG r = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, KEY_SET_VALUE, &hKey);
	if (r != ERROR_SUCCESS)
		return FALSE;

	r = RegDeleteValue(hKey, lpszValueName);
	RegCloseKey(hKey);
	return (r == ERROR_SUCCESS);
}

BOOL GetRegistryStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
							LPCTSTR pszValueName, std::wstring &out) {
	HKEY hKey;
	LONG r = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, KEY_QUERY_VALUE, &hKey);
	if (r != ERROR_SUCCESS)
		return FALSE;

	DWORD dwType = 0;
	DWORD dwSize = 0;

	// 第一次调用获取大小（字节数）
	r = RegQueryValueEx(hKey, pszValueName, nullptr, &dwType, nullptr, &dwSize);
	if (r != ERROR_SUCCESS || (dwType != REG_SZ && dwType != REG_EXPAND_SZ)) {
		RegCloseKey(hKey);
		return FALSE;
	}

	// 分配足够的空间（以 wchar_t 为单位）
	std::wstring buffer(dwSize / sizeof(wchar_t), L'\0');

	// 实际读取
	r = RegQueryValueEx(hKey, pszValueName, nullptr, nullptr,
						reinterpret_cast<LPBYTE>(&buffer[0]), &dwSize);
	RegCloseKey(hKey);

	if (r == ERROR_SUCCESS) {
		// 去掉可能存在的尾部 null
		size_t len = wcsnlen(buffer.c_str(), buffer.size());
		buffer.resize(len);
		out = std::move(buffer);
		return TRUE;
	}

	return FALSE;
}

BOOL GetRegistryDWordValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
						   LPCTSTR pszValueName, DWORD &dwValue) {
	HKEY hKey;
	LONG r = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, KEY_QUERY_VALUE, &hKey);
	if (r != ERROR_SUCCESS)
		return FALSE;

	DWORD dwType = 0, cbData = sizeof(DWORD);
	r = RegQueryValueEx(hKey, pszValueName, NULL, &dwType, (LPBYTE)&dwValue,
						&cbData);
	RegCloseKey(hKey);
	return (r == ERROR_SUCCESS && dwType == REG_DWORD);
}

HKEY get_hkey(const std::string &str) {
	HKEY a = 0;
	if (str == "HKCR") {
		a = HKEY_CLASSES_ROOT;
	} else if (str == "HKCU") {
		a = HKEY_CURRENT_USER;
	} else if (str == "HKLM") {
		a = HKEY_LOCAL_MACHINE;
	} else if (str == "HKU") {
		a = HKEY_USERS;
	}
	return a;
}

/*
typ 类型 普通服务一般用SERVICE_WIN32_OWN_PROCESS
SERVICE_FILE_SYSTEM_DRIVER 文件驱动
SERVICE_KERNEL_DRIVER 内核驱动

// begin_wdm
//
// Service Types (Bit Mask)
//
#define SERVICE_KERNEL_DRIVER          0x00000001
#define SERVICE_FILE_SYSTEM_DRIVER     0x00000002
#define SERVICE_ADAPTER                0x00000004
#define SERVICE_RECOGNIZER_DRIVER      0x00000008

#define SERVICE_DRIVER                 (SERVICE_KERNEL_DRIVER | \
                                        SERVICE_FILE_SYSTEM_DRIVER | \
                                        SERVICE_RECOGNIZER_DRIVER)

#define SERVICE_WIN32_OWN_PROCESS      0x00000010
#define SERVICE_WIN32_SHARE_PROCESS    0x00000020
#define SERVICE_WIN32                  (SERVICE_WIN32_OWN_PROCESS | \
                                        SERVICE_WIN32_SHARE_PROCESS)

#define SERVICE_USER_SERVICE           0x00000040
#define SERVICE_USERSERVICE_INSTANCE   0x00000080

#define SERVICE_USER_SHARE_PROCESS     (SERVICE_USER_SERVICE | \
                                        SERVICE_WIN32_SHARE_PROCESS)
#define SERVICE_USER_OWN_PROCESS       (SERVICE_USER_SERVICE | \
                                        SERVICE_WIN32_OWN_PROCESS)

#define SERVICE_INTERACTIVE_PROCESS    0x00000100
#define SERVICE_PKG_SERVICE            0x00000200

#define SERVICE_TYPE_ALL               (SERVICE_WIN32  | \
                                        SERVICE_ADAPTER | \
                                        SERVICE_DRIVER  | \
                                        SERVICE_INTERACTIVE_PROCESS | \
                                        SERVICE_USER_SERVICE | \
                                        SERVICE_USERSERVICE_INSTANCE | \
                                        SERVICE_PKG_SERVICE)
*/
bool create_service(const std::string &name, const std::string &display_name,
					const std::string &description, const std::string &bin_path,
					int typ,		// 0 = Win32, 1 = driver
					int start_mode, // 0 = auto, 1 = delayed auto, 2 = manual, 3
									// = disabled 4 = boot
					std::string &err) {
	SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
	if (!scm) {
		DWORD code = GetLastError();
		std::ostringstream oss;
		oss << "OpenSCManager failed: " << code;
		err = oss.str();
		return false;
	}

	DWORD service_type = typ;
		//(typ == 1) ? SERVICE_KERNEL_DRIVER : SERVICE_WIN32_OWN_PROCESS;

	DWORD start_type;
	switch (start_mode) {
	case 0:
		start_type = SERVICE_AUTO_START;
		break;
	case 1:
		start_type = SERVICE_AUTO_START;
		break; // 延迟启动标记后面设置
	case 2:
		start_type = SERVICE_DEMAND_START;
		break;
	case 3:
		start_type = SERVICE_DISABLED;
		break;
	case 4:
		start_type = SERVICE_BOOT_START;
		break;
	default:
		err = "Invalid start_mode";
		CloseServiceHandle(scm);
		return false;
	}

	SC_HANDLE svc = CreateServiceW(
		scm, UTF8ToUTF16(name).c_str(), UTF8ToUTF16(display_name).c_str(),
		SERVICE_ALL_ACCESS, service_type, start_type, SERVICE_ERROR_NORMAL,
		UTF8ToUTF16(bin_path).c_str(), nullptr, nullptr, nullptr, nullptr,
		nullptr);

	if (!svc) {
		DWORD code = GetLastError();
		std::ostringstream oss;
		oss << "CreateService failed: " << code;
		err = oss.str();
		CloseServiceHandle(scm);
		return false;
	}

	// 设置描述（仅限普通服务）
	if (typ == 0 && !description.empty()) {
		SERVICE_DESCRIPTIONW desc = {};
		std::wstring wdesc = UTF8ToUTF16(description);
		desc.lpDescription = &wdesc[0];
		ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &desc);
	}

	// 设置延迟自动启动（仅对普通服务有效）
	if (typ == 0 && start_mode == 1) {
		SERVICE_DELAYED_AUTO_START_INFO delayed = {TRUE};
		ChangeServiceConfig2W(svc, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
							  &delayed);
	}

	CloseServiceHandle(svc);
	CloseServiceHandle(scm);
	return true;
}

bool change_service_failure_restart(
	const std::string &name,
	int delay,
	std::string &err) {
	SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!scm) {
		err = "OpenSCManager failed: " + std::to_string(GetLastError());
		return false;
	}

	SC_HANDLE svc = OpenServiceW(scm, UTF8ToUTF16(name).c_str(),
								 SERVICE_ALL_ACCESS);
	if (!svc) {
		err = "OpenService failed: " + std::to_string(GetLastError());
		CloseServiceHandle(scm);
		return false;
	}


	// 配置失败操作：崩溃时 delay 毫秒后重启服务
    SC_ACTION actions[1];
    actions[0].Type = SC_ACTION_RESTART; // 操作：重启服务
    actions[0].Delay = delay;             // 延迟毫秒

    SERVICE_FAILURE_ACTIONSW sfa;
    ZeroMemory(&sfa, sizeof(sfa));
    sfa.dwResetPeriod = 0;               // 不重置失败计数
    sfa.cActions = 1;                    // 只有一个动作
    sfa.lpsaActions = actions;           // 动作数组
	

	// 修改修改崩溃重启参数
	if (!ChangeServiceConfig2W(svc, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa)) {
		err = "ChangeServiceConfig2W (SERVICE_CONFIG_FAILURE_ACTIONS) failed: " + std::to_string(GetLastError());
		CloseServiceHandle(svc);
		CloseServiceHandle(scm);
		return false;
	}

	SERVICE_FAILURE_ACTIONS_FLAG flag;
    flag.fFailureActionsOnNonCrashFailures = TRUE;
    if (!ChangeServiceConfig2W(svc, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &flag)) {
        err = "ChangeServiceConfig2W (SERVICE_CONFIG_FAILURE_ACTIONS_FLAG) failed: " + std::to_string(GetLastError());
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return false;
    }

	CloseServiceHandle(svc);
	CloseServiceHandle(scm);
	return true;
}

bool change_service_start_mode(
	const std::string &name,
	int start_mode, // 0 = auto, 1 = delayed auto, 2 = manual, 3 = disabled, 4 =
					// boot kernel专用
	std::string &err) {
	SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!scm) {
		DWORD code = GetLastError();
		std::ostringstream oss;
		oss << "OpenSCManager failed: " << code;
		err = oss.str();
		return false;
	}

	SC_HANDLE svc = OpenServiceW(scm, UTF8ToUTF16(name).c_str(),
								 SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG);
	if (!svc) {
		DWORD code = GetLastError();
		std::ostringstream oss;
		oss << "OpenService failed: " << code;
		err = oss.str();
		CloseServiceHandle(scm);
		return false;
	}

	DWORD start_type;
	switch (start_mode) {
	case 0:
		start_type = SERVICE_AUTO_START;
		break;
	case 1:
		start_type = SERVICE_AUTO_START;
		break; // 延迟启动后面设置
	case 2:
		start_type = SERVICE_DEMAND_START;
		break;
	case 3:
		start_type = SERVICE_DISABLED;
		break;
	case 4:
		start_type = SERVICE_BOOT_START;
		break;
	default:
		err = "Invalid start_mode";
		CloseServiceHandle(svc);
		CloseServiceHandle(scm);
		return false;
	}

	// 修改启动类型
	if (!ChangeServiceConfigW(svc, SERVICE_NO_CHANGE, start_type,
							  SERVICE_NO_CHANGE, nullptr, nullptr, nullptr,
							  nullptr, nullptr, nullptr, nullptr)) {
		DWORD code = GetLastError();
		std::ostringstream oss;
		oss << "ChangeServiceConfig failed: " << code;
		err = oss.str();
		CloseServiceHandle(svc);
		CloseServiceHandle(scm);
		return false;
	}

	// 设置延迟自动启动（如果需要）
	if (start_mode == 1) {
		SERVICE_DELAYED_AUTO_START_INFO delayed = {TRUE};
		if (!ChangeServiceConfig2W(svc, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
								   &delayed)) {
			DWORD code = GetLastError();
			std::ostringstream oss;
			oss << "Set delayed auto start failed: " << code;
			err = oss.str();
			// 不立即返回，继续关闭句柄
			CloseServiceHandle(svc);
			CloseServiceHandle(scm);
			return false;
		}
	} else {
		// 关闭延迟启动标志（如从 delayed auto 改为其他模式）
		SERVICE_DELAYED_AUTO_START_INFO delayed = {FALSE};
		ChangeServiceConfig2W(svc, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
							  &delayed);
	}

	CloseServiceHandle(svc);
	CloseServiceHandle(scm);
	return true;
}

bool wait_service_status(SC_HANDLE svc, DWORD desired_state, std::string &err,
						 DWORD timeout_ms = 10000) {
	SERVICE_STATUS_PROCESS status = {};
	DWORD bytes_needed = 0;
	DWORD elapsed = 0;

	while (true) {
		if (!QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
								  reinterpret_cast<LPBYTE>(&status),
								  sizeof(status), &bytes_needed)) {
			err = "QueryServiceStatusEx failed: " +
				  std::to_string(GetLastError());
			return false;
		}

		if (status.dwCurrentState == desired_state)
			return true;

		if (status.dwCurrentState == SERVICE_STOP_PENDING ||
			status.dwCurrentState == SERVICE_START_PENDING ||
			status.dwCurrentState == SERVICE_RUNNING) {
			Sleep(200);
			elapsed += 200;
			if (elapsed >= timeout_ms) {
				err = "Timeout waiting for service state change";
				return false;
			}
		} else {
			err = "Unexpected service state: " +
				  std::to_string(status.dwCurrentState);
			return false;
		}
	}
}

bool start_service(const std::string &name, std::string &err) {
	SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!scm) {
		err = "OpenSCManager failed: " + std::to_string(GetLastError());
		return false;
	}

	SC_HANDLE svc = OpenServiceW(scm, UTF8ToUTF16(name).c_str(),
								 SERVICE_START | SERVICE_QUERY_STATUS);
	if (!svc) {
		err = "OpenService failed: " + std::to_string(GetLastError());
		CloseServiceHandle(scm);
		return false;
	}

	if (!StartServiceW(svc, 0, nullptr)) {
		DWORD code = GetLastError();
		if (code != ERROR_SERVICE_ALREADY_RUNNING) {
			err = "StartService failed: " + std::to_string(code);
			CloseServiceHandle(svc);
			CloseServiceHandle(scm);
			return false;
		}
	}

	bool ok = wait_service_status(svc, SERVICE_RUNNING, err, 60000);

	CloseServiceHandle(svc);
	CloseServiceHandle(scm);
	return ok;
}

bool stop_service(const std::string &name, std::string &err) {
	SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!scm) {
		err = "OpenSCManager failed: " + std::to_string(GetLastError());
		return false;
	}

	SC_HANDLE svc = OpenServiceW(scm, UTF8ToUTF16(name).c_str(),
								 SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (!svc) {
		DWORD code = GetLastError();
		if (code == ERROR_SERVICE_DOES_NOT_EXIST) {
			// 服务不存在，不算错误
			CloseServiceHandle(scm);
			return true;
		}
		err = "OpenService failed: " + std::to_string(code);
		CloseServiceHandle(scm);
		return false;
	}

	SERVICE_STATUS status = {};
	if (!ControlService(svc, SERVICE_CONTROL_STOP, &status)) {
		DWORD code = GetLastError();
		if (code != ERROR_SERVICE_NOT_ACTIVE) {
			err = "ControlService failed: " + std::to_string(code);
			CloseServiceHandle(svc);
			CloseServiceHandle(scm);
			return false;
		}
	}

	bool ok = wait_service_status(svc, SERVICE_STOPPED, err, 60000);

	CloseServiceHandle(svc);
	CloseServiceHandle(scm);
	return ok;
}

bool query_service_status(const std::string &name, int &status_out,
						  std::string &err) {
	SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!scm) {
		err = "OpenSCManager failed: " + std::to_string(GetLastError());
		return false;
	}

	SC_HANDLE svc =
		OpenServiceW(scm, UTF8ToUTF16(name).c_str(), SERVICE_QUERY_STATUS);
	if (!svc) {
		DWORD code = GetLastError();
		if (code == ERROR_SERVICE_DOES_NOT_EXIST) {
			// 服务不存在，不算错误
			status_out = 0xff;
			CloseServiceHandle(scm);
			return true;
		}
		err = "OpenService failed: " + std::to_string(code);
		CloseServiceHandle(scm);
		return false;
	}

	SERVICE_STATUS_PROCESS status = {};
	DWORD needed = 0;
	if (!QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&status,
							  sizeof(status), &needed)) {
		err = "QueryServiceStatusEx failed: " + std::to_string(GetLastError());
		CloseServiceHandle(svc);
		CloseServiceHandle(scm);
		return false;
	}

	status_out = status.dwCurrentState;

	CloseServiceHandle(svc);
	CloseServiceHandle(scm);
	return true;
}
// 枚举指定进程名的所有进程 ID（忽略大小写）
static bool get_process_ids_by_name(const std::wstring &process_name,
									std::vector<DWORD> &pids) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return false;

	PROCESSENTRY32W pe;
	pe.dwSize = sizeof(pe);

	if (Process32FirstW(hSnapshot, &pe)) {
		do {
			if (_wcsicmp(pe.szExeFile, process_name.c_str()) == 0) {
				pids.push_back(pe.th32ProcessID);
			}
		} while (Process32NextW(hSnapshot, &pe));
	}

	CloseHandle(hSnapshot);
	return !pids.empty();
}

// 强制结束指定名称的进程（忽略大小写），并等待其退出
bool kill_process_by_name(const std::string &name_utf8, uint32_t timeout_ms,
						  std::string &err) {
	std::wstring name = UTF8ToUTF16(name_utf8);
	std::vector<DWORD> pids;
	if (!get_process_ids_by_name(name, pids)) {
		err = "No process found with name: " + name_utf8;
		return true;
	}

	std::vector<HANDLE> handles;
	for (DWORD pid : pids) {
		HANDLE hProcess =
			OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
		if (!hProcess) {
			continue; // 无权限或已退出
		}
		TerminateProcess(hProcess, 1);
		handles.push_back(hProcess);
	}

	// 等待所有进程结束
	auto start = std::chrono::steady_clock::now();
	for (HANDLE h : handles) {
		DWORD elapsed = static_cast<DWORD>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start)
				.count());

		DWORD wait_time = (elapsed >= timeout_ms) ? 0 : (timeout_ms - elapsed);
		WaitForSingleObject(h, wait_time);
		CloseHandle(h);
	}

	return true;
}

bool delete_service(const std::string &name, std::string &err) {
	SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!scm) {
		DWORD code = GetLastError();
		std::ostringstream oss;
		oss << "OpenSCManager failed: " << code;
		err = oss.str();
		return false;
	}

	SC_HANDLE svc = OpenServiceW(scm, UTF8ToUTF16(name).c_str(), DELETE);
	if (!svc) {
		DWORD code = GetLastError();
		if (code == ERROR_SERVICE_DOES_NOT_EXIST) {
			// 服务不存在，不算错误
			CloseServiceHandle(scm);
			return true;
		}
		std::ostringstream oss;
		oss << "OpenService failed: " << code;
		err = oss.str();
		CloseServiceHandle(scm);
		return false;
	}

	BOOL ok = DeleteService(svc);
	if (!ok) {
		DWORD code = GetLastError();
		std::ostringstream oss;
		oss << "DeleteService failed: " << code;
		err = oss.str();
		CloseServiceHandle(svc);
		CloseServiceHandle(scm);
		return false;
	}

	CloseServiceHandle(svc);
	CloseServiceHandle(scm);
	return true;
}

BOOL IsPhysicalAdapter(const IP_ADAPTER_ADDRESSES *pAdapter) {
	// 排除回环接口
	if (pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
		return FALSE;

	// 排除隧道接口（VPN等）
	if (pAdapter->IfType == IF_TYPE_TUNNEL)
		return FALSE;

	// 排除 PPP 接口（如PPTP VPN）
	if (pAdapter->IfType == IF_TYPE_PPP)
		return FALSE;

	//// 常见物理网卡类型
	// const DWORD physical_types[] = {
	//     IF_TYPE_ETHERNET_CSMACD,  // 以太网
	//     IF_TYPE_IEEE80211,        // 无线网络
	//     IF_TYPE_ATM,              // ATM
	//     IF_TYPE_IEEE1394          // 火线
	// };

	// for (auto type : physical_types) {
	//     if (pAdapter->IfType == type) {
	//         return TRUE;
	//     }
	// }

	// 通过名称关键词二次过滤
	const std::wstring desc = pAdapter->Description;
	const std::wstring keywords[] = {
		L"vEthernet", L"Loopback", L"VMware",	 L"VirtualBox",	 L"WSL",
		L"TAP",		  L"Hyper-V",  L"Bluetooth", L"Virtual",	 L"VPN",
		L"Docker",	  L"本地连接", L"隧道",		 L"Tunnel",		 L"Adapter",
		L"Microsoft", L"Remote",   L"Pseudo",	 L"WAN Miniport"};
	// const std::wstring keywords[] = { L"Virtual", L"VPN", L"Hyper-V",
	// L"VMware", L"TAP" };

	for (const auto &kw : keywords) {
		if (desc.find(kw) != std::wstring::npos) {
			return FALSE;
		}
	}

	return TRUE;
}

std::string ByteArrayToMacString(const BYTE *macBytes) {
	const char hexDigits[] = "0123456789ABCDEF";
	std::string macStr; // 输出字符串（长度固定为17字符）

	for (int i = 0; i < 6; ++i) { // 仅处理前6字节
		if (i != 0) {
			macStr += ':';
		}
		// 高4位和低4位分别转换为字符
		BYTE b = macBytes[i];
		macStr += hexDigits[(b >> 4) & 0x0F];
		macStr += hexDigits[b & 0x0F];
	}
	return macStr;
}

/* 获取本机真实MAC地址，排除虚拟网卡等 */
BOOL GetPhysicalMacAddressList(std::vector<std::string> &mac_addresses) {
	mac_addresses.clear();

	ULONG outBufLen = 0;
	DWORD dwRetVal = 0;

	// 第一次调用获取所需缓冲区大小
	if (GetAdaptersAddresses(AF_INET, 0, NULL, NULL, &outBufLen) ==
		ERROR_BUFFER_OVERFLOW) {
		PIP_ADAPTER_ADDRESSES pAddresses =
			(PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
		if (!pAddresses)
			return false;

		// 获取所有适配器信息
		if ((dwRetVal = GetAdaptersAddresses(AF_INET, 0, NULL, pAddresses,
											 &outBufLen)) == ERROR_SUCCESS) {
			for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL;
				 pCurr = pCurr->Next) {
				// 检查适配器状态和类型
				if (pCurr->OperStatus != IfOperStatusUp ||
					!IsPhysicalAdapter(pCurr)) {
					continue;
				}
				std::string macstr =
					ByteArrayToMacString(pCurr->PhysicalAddress);
				mac_addresses.emplace_back(macstr);
			}
		}
		free(pAddresses);
	}
	return !mac_addresses.empty();
}

/* 获取本机真实IP地址，排除虚拟网卡等 */
BOOL GetPhysicalIpAddressList(std::vector<std::string> &ip_addresses) {
	ip_addresses.clear();

	ULONG outBufLen = 0;
	DWORD dwRetVal = 0;

	// 第一次调用获取所需缓冲区大小
	if (GetAdaptersAddresses(AF_INET, 0, NULL, NULL, &outBufLen) ==
		ERROR_BUFFER_OVERFLOW) {
		PIP_ADAPTER_ADDRESSES pAddresses =
			(PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
		if (!pAddresses)
			return false;

		// 获取所有适配器信息
		if ((dwRetVal = GetAdaptersAddresses(AF_INET, 0, NULL, pAddresses,
											 &outBufLen)) == ERROR_SUCCESS) {
			for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL;
				 pCurr = pCurr->Next) {
				// 检查适配器状态和类型
				if (pCurr->OperStatus != IfOperStatusUp ||
					!IsPhysicalAdapter(pCurr)) {
					continue;
				}
				// 遍历IP地址
				for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast =
						 pCurr->FirstUnicastAddress;
					 pUnicast != NULL; pUnicast = pUnicast->Next) {
					if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
						char ipstr[INET_ADDRSTRLEN] = {0};
						inet_ntop(AF_INET,
								  &((struct sockaddr_in *)
										pUnicast->Address.lpSockaddr)
									   ->sin_addr,
								  ipstr, sizeof(ipstr));
						ip_addresses.emplace_back(ipstr);
					}
				}
			}
		}
		free(pAddresses);
	}
	return !ip_addresses.empty();
}

// 计算字符串的 SHA-256 哈希值
std::string CalculateSHA256(const std::string &input) {
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE *pbHash = NULL;
	DWORD dwHashLen = 0;

	// 初始化加密服务提供程序
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES,
							 CRYPT_VERIFYCONTEXT)) {
		return "";
	}

	// 创建哈希对象（SHA-256）
	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
		CryptReleaseContext(hProv, 0);
		return "";
	}

	// 添加数据到哈希
	if (!CryptHashData(hHash, (const BYTE *)input.data(), (DWORD)input.size(),
					   0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return "";
	}

	// 获取哈希值长度
	DWORD dwHashSize = 0;
	DWORD dwSize = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&dwHashSize, &dwSize,
						   0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return "";
	}

	// 提取哈希值
	std::vector<BYTE> buffer(dwHashSize);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, buffer.data(), &dwHashSize, 0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return "";
	}

	// 转换为十六进制字符串
	std::stringstream ss;
	for (const auto &b : buffer) {
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
	}

	// 清理资源
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	return ss.str();
}

BOOL BroadcastEnvironmentChange() {
	// 广播给所有窗口
	DWORD_PTR dwResult = 0;
	LRESULT res = SendMessageTimeout(
        HWND_BROADCAST,
        WM_SETTINGCHANGE,
        0,
        (LPARAM)L"Environment", // 必须是 Environment
        SMTO_ABORTIFHUNG,
        5000,
        &dwResult
    );
	return res != 0;
}

} // namespace utils