#include "utils_res.h"
#include <fstream>

namespace utils {
bool update_exe_resource_with_file(const std::string &exe_path,
								   const std::string &file_path,
								   WORD resource_id,
								   const std::string &resource_type,
								   std::string &err) {
	// 读取 zip 文件内容
	std::ifstream file(file_path, std::ios::binary);
	if (!file) {
		err = "Failed to open zip file: " + file_path;
		return false;
	}

	std::vector<char> zip_data((std::istreambuf_iterator<char>(file)),
							   std::istreambuf_iterator<char>());

	// 转换路径和资源类型为宽字符串
	std::wstring exe_path_w(exe_path.begin(), exe_path.end());
	std::wstring resource_type_w(resource_type.begin(), resource_type.end());

	// 开始更新资源
	HANDLE update_handle = BeginUpdateResourceW(exe_path_w.c_str(), FALSE);
	if (!update_handle) {
		err = "BeginUpdateResource failed. Error code: " +
			  std::to_string(GetLastError());
		return false;
	}

	BOOL result = UpdateResourceW(
		update_handle, resource_type_w.c_str(), MAKEINTRESOURCEW(resource_id),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), zip_data.data(),
		static_cast<DWORD>(zip_data.size()));

	if (!result) {
		err = "UpdateResource failed. Error code: " +
			  std::to_string(GetLastError());
		EndUpdateResourceW(update_handle, TRUE); // 放弃更改
		return false;
	}

	if (!EndUpdateResourceW(update_handle, FALSE)) {
		err = "EndUpdateResource failed. Error code: " +
			  std::to_string(GetLastError());
		return false;
	}

	err.clear(); // 没有错误
	return true;
}

#pragma pack(push, 1)
struct ICONDIR {
	WORD id_reserved;
	WORD id_type;
	WORD id_count;
};

struct ICONDIRENTRY {
	BYTE b_width;
	BYTE b_height;
	BYTE b_color_count;
	BYTE b_reserved;
	WORD w_planes;
	WORD w_bit_count;
	DWORD dw_bytes_in_res;
	DWORD dw_image_offset;
};

struct GRPICONDIR {
	WORD id_reserved;
	WORD id_type;
	WORD id_count;
};

struct GRPICONDIRENTRY {
	BYTE b_width;
	BYTE b_height;
	BYTE b_color_count;
	BYTE b_reserved;
	WORD w_planes;
	WORD w_bit_count;
	DWORD dw_bytes_in_res;
	WORD n_id;
};

#pragma pack(pop)

bool update_exe_icon(const std::string &exe_path, const std::string &ico_path,
					 std::string &err) {
	// 读取 .ico 文件
	std::ifstream file(ico_path, std::ios::binary);
	if (!file) {
		err = "Failed to open .ico file.";
		return false;
	}

	std::vector<char> ico_data((std::istreambuf_iterator<char>(file)),
							   std::istreambuf_iterator<char>());
	if (ico_data.size() < sizeof(ICONDIR)) {
		err = "Invalid .ico file.";
		return false;
	}

	const ICONDIR *icon_dir =
		reinterpret_cast<const ICONDIR *>(ico_data.data());
	if (icon_dir->id_type != 1 || icon_dir->id_reserved != 0) {
		err = "Invalid ICONDIR header.";
		return false;
	}

	const ICONDIRENTRY *icon_entries = reinterpret_cast<const ICONDIRENTRY *>(
		ico_data.data() + sizeof(ICONDIR));
	int icon_count = icon_dir->id_count;

	// 打开EXE准备写资源
	std::wstring exe_path_w(exe_path.begin(), exe_path.end());
	HANDLE h_update = BeginUpdateResourceW(exe_path_w.c_str(), FALSE);
	if (!h_update) {
		err = "BeginUpdateResource failed.";
		return false;
	}

	// 写 RT_ICON
	std::vector<GRPICONDIRENTRY> grp_entries;
	for (int i = 0; i < icon_count; ++i) {
		const ICONDIRENTRY &entry = icon_entries[i];
		if (entry.dw_image_offset + entry.dw_bytes_in_res > ico_data.size()) {
			err = "Corrupted .ico file.";
			EndUpdateResourceW(h_update, TRUE);
			return false;
		}

		const BYTE *img_data = reinterpret_cast<const BYTE *>(ico_data.data()) +
							   entry.dw_image_offset;

		WORD res_id = static_cast<WORD>(i + 1); // 图像ID，从1开始

		BOOL r = UpdateResourceW(h_update, RT_ICON, MAKEINTRESOURCEW(res_id),
								 MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
								 (LPVOID)img_data, entry.dw_bytes_in_res);
		if (!r) {
			err = "Failed to update RT_ICON. Error: " +
				  std::to_string(GetLastError());
			EndUpdateResourceW(h_update, TRUE);
			return false;
		}

		GRPICONDIRENTRY grp_entry = {};
		grp_entry.b_width = entry.b_width;
		grp_entry.b_height = entry.b_height;
		grp_entry.b_color_count = entry.b_color_count;
		grp_entry.b_reserved = entry.b_reserved;
		grp_entry.w_planes = entry.w_planes;
		grp_entry.w_bit_count = entry.w_bit_count;
		grp_entry.dw_bytes_in_res = entry.dw_bytes_in_res;
		grp_entry.n_id = res_id;

		grp_entries.push_back(grp_entry);
	}

	// 构造 RT_GROUP_ICON 结构体数据
	size_t grp_size =
		sizeof(GRPICONDIR) + grp_entries.size() * sizeof(GRPICONDIRENTRY);
	std::vector<BYTE> grp_data(grp_size);

	GRPICONDIR *grp_dir = reinterpret_cast<GRPICONDIR *>(grp_data.data());
	grp_dir->id_reserved = 0;
	grp_dir->id_type = 1;
	grp_dir->id_count = static_cast<WORD>(grp_entries.size());

	std::memcpy(grp_data.data() + sizeof(GRPICONDIR), grp_entries.data(),
				grp_entries.size() * sizeof(GRPICONDIRENTRY));

	// 更新 RT_GROUP_ICON（通常资源ID = 1）
	BOOL res =
		UpdateResourceW(h_update, RT_GROUP_ICON, MAKEINTRESOURCEW(1),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
						grp_data.data(), static_cast<DWORD>(grp_data.size()));

	if (!res) {
		err = "Failed to update RT_GROUP_ICON. Error: " +
			  std::to_string(GetLastError());
		EndUpdateResourceW(h_update, TRUE);
		return false;
	}

	if (!EndUpdateResourceW(h_update, FALSE)) {
		err = "EndUpdateResource failed. Error: " +
			  std::to_string(GetLastError());
		return false;
	}

	err.clear();
	return true;
}

bool update_exe_resource_with_mem(const std::string &exe_path, void *buffer,
								  size_t buffer_len, WORD resource_id,
								  const std::string &resource_type,
								  std::string &err) {
	// 转换路径和资源类型为宽字符串
	std::wstring exe_path_w(exe_path.begin(), exe_path.end());
	std::wstring resource_type_w(resource_type.begin(), resource_type.end());

	// 开始更新资源
	HANDLE update_handle = BeginUpdateResourceW(exe_path_w.c_str(), FALSE);
	if (!update_handle) {
		err = "BeginUpdateResource failed. Error code: " +
			  std::to_string(GetLastError());
		return false;
	}

	BOOL result = UpdateResourceW(update_handle, resource_type_w.c_str(),
								  MAKEINTRESOURCEW(resource_id),
								  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
								  buffer, static_cast<DWORD>(buffer_len));

	if (!result) {
		err = "UpdateResource failed. Error code: " +
			  std::to_string(GetLastError());
		EndUpdateResourceW(update_handle, TRUE); // 放弃更改
		return false;
	}

	if (!EndUpdateResourceW(update_handle, FALSE)) {
		err = "EndUpdateResource failed. Error code: " +
			  std::to_string(GetLastError());
		return false;
	}

	err.clear(); // 没有错误
	return true;
}
} // namespace utils