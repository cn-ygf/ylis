#pragma once

#pragma pack(push, 1)
struct ylis_header {
	char magic[4]; // 'YLIS'
	char m;
	char j;
	uint64_t offset;  // 安装数据起始偏移
	uint64_t size;	  // 安装数据大小
	uint8_t hash[16]; // md5校验
};
#pragma pack(pop)

void self_delete(const std::string &path);
bool release_install_data_from_pe(
	const std::string &pe_file, uint64_t offset, uint64_t size,
	const std::string &install_path, std::string &err,
	std::function<void(int)> progress_cb = nullptr);
bool locate_install_data(const std::string &pe_file, ylis_header &header,
						 std::string &err);
bool check_install_data(const std::string &pe_file, ylis_header *header,
						std::string &err);
bool get_resource(WORD resource_id, const std::string &resource_type,
				  std::string &out, std::string &err);
bool writer_uninstall_file(const std::string &pe_file, ylis_header *header,
						   const std::string &installdir, std::string &err);