#pragma once

#include "lualib/func.h"
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
bool build_install_with_zip(ylis_build_state *p, const std::string &out,
							std::string &err);
bool writer_install_data(const std::string &pe_file,
						 const std::string &install_data_file,
						 std::string &err);
bool write_version_res_data(const std::string &pe_file,
							const std::string &version,
							const std::string &product,
							const std::string &company, std::string &err);