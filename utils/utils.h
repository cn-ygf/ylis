#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace utils {
std::string CalculateSHA256(const std::string &input);
std::string CalculateMD5(const uint8_t *data, unsigned long size);
std::string CalculateFileMD5(const std::string &filename);
bool CalculateFileMD5(const std::string &filename, unsigned char digest[16]);
std::string MakeAbsolutePath(const std::string &path);
std::string MakeAbsolutePath(const std::string &path,
							 const std::string &base_path);
bool IsDriveLetterAvailable(wchar_t driveLetter);
bool IsPathSyntaxValid(const std::wstring &path);
std::string LocalToUTF8(const std::string &local);
std::string UTF8ToLocal(const std::string &utf8);
std::string UTF16ToLocal(const std::wstring &wstr);
std::wstring LocalToUTF16(const std::string &str);
std::wstring UTF8ToUTF16(const std::string &utf8);
std::string UTF16ToUTF8(const std::wstring &utf16);
bool exec_command(const std::string &command, std::string &stdOut,
				  std::string &stdErr);
bool create_process(const std::string &command);
bool IsRunningAsSystem();
HKEY get_hkey(const std::string &str);
BOOL DeleteRegistryValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
						 LPCTSTR lpszValueName);
/*
 * 从注册表获取字符串类型的值
 * @hKeyParent 打开的项的句柄
 * @lpszKeyName 指定要创建或打开的项的名称。 此名称必须是 hKeyParent 的子项
 * @pszValueName 指向以 NULL 结尾的字符串的指针，该字符串包含要查询的值的名称
 * @out 输出的字符串。
 */
BOOL GetRegistryStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
							LPCTSTR pszValueName, std::wstring &out);
/*
 * 删除注册表的项
 * @hKeyParent 打开的项的句柄
 * @lpszKeyName 要删除的项的名称
 */
BOOL DeleteRegistryKey(HKEY hKeyParent, LPCTSTR lpszKeyName);
/*
 * 从注册表获取DWORD类型的值
 * @hKeyParent 打开的项的句柄
 * @lpszKeyName 指定要创建或打开的项的名称。 此名称必须是 hKeyParent 的子项
 * @pszValueName 指向以 NULL 结尾的字符串的指针，该字符串包含要查询的值的名称
 * @dwValue 接收返回值。
 */
BOOL GetRegistryDWordValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
						   LPCTSTR pszValueName, DWORD &dwValue);

/*
 * 从注册表写入字符串类型的值
 * @hKeyParent 打开的项的句柄
 * @lpszKeyName 指定要创建或打开的项的名称。 此名称必须是 hKeyParent 的子项
 * @pszValueName 指向以 NULL 结尾的字符串的指针，该字符串包含要查询的值的名称
 * @pszValue 指向要用指定的值名称存储的字符串数据的指针
 */
BOOL SetRegistryStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
							LPCTSTR pszValueName, LPCTSTR pszValue);
/*
 * 从注册表写入带环境变量的字符串类型的值
 * @hKeyParent 打开的项的句柄
 * @lpszKeyName 指定要创建或打开的项的名称。 此名称必须是 hKeyParent 的子项
 * @pszValueName 指向以 NULL 结尾的字符串的指针，该字符串包含要查询的值的名称
 * @pszValue 指向要用指定的值名称存储的字符串数据的指针
 */
BOOL SetRegistryStringValueEx(HKEY hKeyParent, LPCTSTR lpszKeyName,
							  LPCTSTR pszValueName, LPCTSTR pszValue);
/*
 * 从注册表写入多项字符串类型的值
 * @hKeyParent 打开的项的句柄
 * @lpszKeyName 指定要创建或打开的项的名称。 此名称必须是 hKeyParent 的子项
 * @pszValueName 指向以 NULL 结尾的字符串的指针，该字符串包含要查询的值的名称
 * @pszValue 指向要用指定的值名称存储的字符串数据的指针
 */
BOOL SetRegistryMultiStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
								 LPCTSTR pszValueName, LPCTSTR pszValue);

/*
 * 从注册表写入DWORD串类型的值
 * @hKeyParent 打开的项的句柄
 * @lpszKeyName 指定要创建或打开的项的名称。 此名称必须是 hKeyParent 的子项
 * @pszValueName 指向以 NULL 结尾的字符串的指针，该字符串包含要查询的值的名称
 * @dwValue 值
 */
BOOL SetRegistryDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
						   LPCTSTR pszValueName, DWORD dwValue);

bool create_service(
	const std::string &name, const std::string &display_name,
	const std::string &description, const std::string &bin_path,
	int typ,		// 0 = Win32, 1 = driver
	int start_mode, // 0 = auto, 1 = delayed auto, 2 = manual, 3 = disabled
	std::string &err);
bool change_service_start_mode(
	const std::string &name,
	int start_mode, // 0 = auto, 1 = delayed auto, 2 = manual, 3 = disabled
	std::string &err);
bool start_service(const std::string &name, std::string &err);
bool stop_service(const std::string &name, std::string &err);
bool delete_service(const std::string &name, std::string &err);
bool query_service_status(const std::string &name, int &status_out,
						  std::string &err);
bool kill_process_by_name(const std::string &name_utf8, uint32_t timeout_ms,
						  std::string &err);
/* 获取本机真实MAC地址，排除虚拟网卡等 */
BOOL GetPhysicalMacAddressList(std::vector<std::string> &mac_addresses);
BOOL GetPhysicalIpAddressList(std::vector<std::string> &ip_addresses);

} // namespace utils