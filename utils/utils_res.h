#pragma once
#include <map>
#include <string>
#include <vector>
#include <windows.h>

namespace utils {
bool update_exe_resource_with_file(const std::string &exe_path,
								   const std::string &file_path,
								   WORD resource_id,
								   const std::string &resource_type,
								   std::string &err);
bool update_exe_icon(const std::string &exe_path, const std::string &ico_path,
					 std::string &err);
bool update_exe_resource_with_mem(const std::string &exe_path, void *buffer,
								  size_t buffer_len, WORD resource_id,
								  const std::string &resource_type,
								  std::string &err);
} // namespace utils