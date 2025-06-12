#pragma once

#include <string>
#include <map>

namespace utils {
/// @brief Updates skin resources in a ZIP archive from memory and writes to a
/// new file.
/// @param buffer The in-memory ZIP archive content.
/// @param update_list Map of file paths inside the ZIP to their replacement
/// contents.
/// @param outfile Path to the output ZIP file.
/// @param err Receives the error message if the function fails.
/// @return true if successful, false otherwise.
bool skin_resource_update(const std::string &buffer,
						  const std::map<std::string, std::string> &update_list,
						  const std::string &outfile, std::string &err);
} // namespace utils