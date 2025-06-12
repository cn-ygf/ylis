#include "loader.h"
#include "pch.h"
#include <direct.h> // Windows mkdir
#include <filesystem>
#include <hv/hlog.h>
#include <hv/md5.h>
#include <hv/base64.h>
#include <io.h>
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <utils/utils.h>
#include <utils/utils_res.h>

// 延迟删除目录，用于uninstall.exe自删
void self_delete(const std::string &path) {
	std::wstring path_w = utils::UTF8ToUTF16(path);
	std::wstring cmd =
		L"cmd /c ping 127.0.0.1 -n 2 > nul && rmdir /Q /S \"" + path_w + L"\"";
	LOGE("DEBUG: %s", utils::UTF16ToUTF8(cmd).c_str());
	STARTUPINFO si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi;
	if (CreateProcess(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE,
					  CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

// 写入卸载文件
bool writer_uninstall_file(const std::string &pe_file, ylis_header *header,
						   const std::string &installdir, std::string &err) {
	// 打开原始 PE 文件
	std::ifstream in(pe_file, std::ios::binary);
	if (!in) {
		err = "Failed to open PE file: " + pe_file;
		return false;
	}

	// 确保 offset 是合理的
	in.seekg(0, std::ios::end);
	uint64_t file_size = static_cast<uint64_t>(in.tellg());
	if (header->offset > file_size) {
		err = "Header offset exceeds file size.";
		return false;
	}
	in.seekg(0, std::ios::beg);

	// 拼接输出路径
	std::filesystem::path out_path =
		std::filesystem::path(installdir) / "uninstall.exe";

	std::ofstream out(out_path, std::ios::binary);
	if (!out) {
		err = "Failed to create uninstall.exe at: " + out_path.string();
		return false;
	}

	// 拷贝 offset 前的数据
	const size_t buffer_size = 8192;
	std::vector<char> buffer(buffer_size);
	uint64_t remaining = header->offset;

	while (remaining > 0) {
		size_t to_read =
			static_cast<size_t>(std::min<uint64_t>(buffer_size, remaining));
		in.read(buffer.data(), to_read);
		std::streamsize read_bytes = in.gcount();
		if (read_bytes <= 0) {
			err = "Failed to read PE file while copying to uninstall.exe.";
			return false;
		}
		out.write(buffer.data(), read_bytes);
		remaining -= read_bytes;
	}
	out.close();

	// 写入安装目录到资源
	std::string install_path_base64 = hv::Base64Encode((const uint8_t*)installdir.data(), installdir.size());
	if (!utils::update_exe_resource_with_mem(out_path.string(), install_path_base64.data(),
											 install_path_base64.size(), IDR_THEME_SET,
											 "THEME", err)) {
		LOGE("PE文件%s 写入脚本失败", out_path.string().c_str());
	}

	return true;
}

// 从pe文件带有魔术标记的地方计算md5
bool check_install_data(const std::string &pe_file, ylis_header *header,
						std::string &err) {
	std::ifstream in(pe_file, std::ios::binary);
	if (!in) {
		err = "Cannot open PE file.";
		return false;
	}
	// 定位到 offset 位置
	in.seekg(0, std::ios::end);
	std::streamsize file_size = in.tellg();
	if (header->offset + header->size > static_cast<uint64_t>(file_size)) {
		err = "Offset + size exceeds file size.";
		return false;
	}
	in.seekg(header->offset, std::ios::beg);

	// 初始化 MD5 计算
	HV_MD5_CTX ctx;
	HV_MD5Init(&ctx);

	std::vector<char> buffer(8192);
	uint64_t remaining = header->size;

	while (remaining > 0) {
		std::streamsize to_read = static_cast<std::streamsize>(
			std::min<uint64_t>(buffer.size(), remaining));
		in.read(buffer.data(), to_read);
		std::streamsize read_bytes = in.gcount();

		if (read_bytes <= 0) {
			err = "Failed to read PE file during MD5 calculation.";
			return false;
		}

		HV_MD5Update(&ctx, reinterpret_cast<unsigned char *>(buffer.data()),
					 static_cast<unsigned int>(read_bytes));
		remaining -= read_bytes;
	}

	uint8_t digest[16] = {0};
	HV_MD5Final(&ctx, digest);

	if (memcmp(digest, header->hash, 16) != 0) {
		err = "MD5 checksum does not match.";
		return false;
	}

	return true;
}

// 从pe文件带有魔术标记的地方读取zip文件的偏移，和文件大小
bool locate_install_data(const std::string &pe_file, ylis_header &header,
						 std::string &err) {
	const size_t header_size = sizeof(ylis_header);
	const size_t chunk_size = 4096;

	std::ifstream in(pe_file, std::ios::binary);
	if (!in) {
		err = "Cannot open PE file.";
		return false;
	}

	// 获取文件大小
	in.seekg(0, std::ios::end);
	std::streamoff file_size = in.tellg();
	in.seekg(0);

	if (file_size < static_cast<std::streamoff>(header_size)) {
		err = "File too small to contain YLIS header.";
		return false;
	}

	std::vector<char> buffer;
	std::streamoff read_pos = 0;
	std::vector<char> prev_tail;

	while (read_pos < file_size) {
		size_t to_read = static_cast<size_t>(
			std::min<std::streamoff>(chunk_size, file_size - read_pos));
		std::vector<char> chunk(to_read);

		in.seekg(read_pos);
		in.read(chunk.data(), to_read);
		if (in.gcount() != static_cast<std::streamsize>(to_read)) {
			err = "Failed to read file chunk.";
			return false;
		}

		if (!prev_tail.empty()) {
			buffer.resize(prev_tail.size() + to_read);
			std::memcpy(buffer.data(), prev_tail.data(), prev_tail.size());
			std::memcpy(buffer.data() + prev_tail.size(), chunk.data(),
						to_read);
		} else {
			buffer = std::move(chunk);
		}

		for (size_t i = 0; i + header_size <= buffer.size(); ++i) {
			if (std::memcmp(buffer.data() + i, "YLIS", 4) == 0) {
				std::streamoff header_pos =
					read_pos - static_cast<std::streamoff>(prev_tail.size()) +
					static_cast<std::streamoff>(i);

				if (header_pos + static_cast<std::streamoff>(header_size) >
					file_size) {
					err = "YLIS header truncated.";
					return false;
				}

				in.seekg(header_pos);
				// ylis_header header{};
				in.read(reinterpret_cast<char *>(&header), header_size);
				if (header.m != 0x09 || header.j != 0x03) {
					continue;
				}
				if (std::memcmp(header.magic, "YLIS", 4) != 0) {
					err = "Invalid magic at header.";
					return false;
				}

				if (header.offset + header.size >
					static_cast<uint64_t>(file_size)) {
					err = "Offset + size exceeds file size.";
					LOGI("Offset: %I64u, Size: %I64u, FileSize: %I64u",
						 header.offset, header.size, file_size);
					return false;
				}

				// offset = header.offset;
				// size = header.size;
				return true;
			}
		}

		if (buffer.size() >= header_size - 1) {
			prev_tail.assign(buffer.end() - (header_size - 1), buffer.end());
		} else {
			prev_tail = buffer;
		}

		read_pos += to_read;
	}

	err = "YLIS header not found.";
	return false;
}

// 从资源里读取数据
bool get_resource(WORD resource_id, const std::string &resource_type,
				  std::string &out, std::string &err) {
	// 当前模块（自身）
	HMODULE h_module = GetModuleHandleW(nullptr);
	if (!h_module) {
		err =
			"GetModuleHandle failed. Error: " + std::to_string(GetLastError());
		return false;
	}

	// 转换资源类型为宽字符串
	std::wstring type_w(resource_type.begin(), resource_type.end());

	// 查找资源
	HRSRC h_res =
		FindResourceW(h_module, MAKEINTRESOURCEW(resource_id), type_w.c_str());
	if (!h_res) {
		err = "FindResource failed. Error: " + std::to_string(GetLastError());
		return false;
	}

	// 获取资源大小
	DWORD size = SizeofResource(h_module, h_res);
	if (size == 0) {
		err = "SizeofResource failed. Error: " + std::to_string(GetLastError());
		return false;
	}

	// 加载资源
	HGLOBAL h_data = LoadResource(h_module, h_res);
	if (!h_data) {
		err = "LoadResource failed. Error: " + std::to_string(GetLastError());
		return false;
	}

	// 锁定资源，获取数据指针
	void *p_data = LockResource(h_data);
	if (!p_data) {
		err = "LockResource failed.";
		return false;
	}

	// 复制到 out
	out.assign(static_cast<const char *>(p_data), size);
	err.clear();
	return true;
}

// 递归创建目录
static void create_directories(const std::string &path) {
	std::string current;
	for (char ch : path) {
		current += ch;
		if (ch == '\\' || ch == '/') {
			if (_access(current.c_str(), 0) != 0)
				_mkdir(current.c_str());
		}
	}
	if (_access(path.c_str(), 0) != 0)
		_mkdir(path.c_str());
}

// 偏移读取结构
typedef struct {
	FILE *real_fp;
	uint64_t base_offset;
	uint64_t size_limit;
	uint64_t position;
} OffsetFile;

// zlib_filefunc_def 文件函数（偏移读取）
static voidpf ZCALLBACK offset_open_file(voidpf opaque, const char *, int) {
	OffsetFile *f = (OffsetFile *)opaque;
	fseek(f->real_fp, static_cast<long>(f->base_offset), SEEK_SET);
	f->position = 0;
	return opaque;
}
static uLong ZCALLBACK offset_read_file(voidpf opaque, voidpf, void *buf,
										uLong size) {
	OffsetFile *f = (OffsetFile *)opaque;
	uint64_t remaining = f->size_limit - f->position;
	if (remaining == 0)
		return 0;
	if (size > remaining)
		size = static_cast<uLong>(remaining);
	size_t read = fread(buf, 1, size, f->real_fp);
	f->position += read;
	return (uLong)read;
}
static long ZCALLBACK offset_tell_file(voidpf opaque, voidpf) {
	OffsetFile *f = (OffsetFile *)opaque;
	return static_cast<long>(f->position);
}
static long ZCALLBACK offset_seek_file(voidpf opaque, voidpf, uLong offset,
									   int origin) {
	OffsetFile *f = (OffsetFile *)opaque;
	uint64_t new_pos = 0;
	switch (origin) {
	case ZLIB_FILEFUNC_SEEK_CUR:
		new_pos = f->position + offset;
		break;
	case ZLIB_FILEFUNC_SEEK_END:
		new_pos = f->size_limit + offset;
		break;
	case ZLIB_FILEFUNC_SEEK_SET:
		new_pos = offset;
		break;
	default:
		return -1;
	}
	if (new_pos > f->size_limit)
		return -1;
	f->position = new_pos;
	return fseek(f->real_fp, static_cast<long>(f->base_offset + new_pos),
				 SEEK_SET);
}
static int ZCALLBACK offset_close_file(voidpf, voidpf) { return 0; }
static int ZCALLBACK offset_error_file(voidpf, voidpf) { return 0; }

static void fill_offset_filefunc(zlib_filefunc_def *pzlib_filefunc_def,
								 OffsetFile *f) {
	pzlib_filefunc_def->zopen_file = offset_open_file;
	pzlib_filefunc_def->zread_file = offset_read_file;
	pzlib_filefunc_def->ztell_file = offset_tell_file;
	pzlib_filefunc_def->zseek_file = offset_seek_file;
	pzlib_filefunc_def->zclose_file = offset_close_file;
	pzlib_filefunc_def->zerror_file = offset_error_file;
	pzlib_filefunc_def->opaque = f;
}

// 主函数
bool release_install_data_from_pe(const std::string &pe_file, uint64_t offset,
								  uint64_t size,
								  const std::string &install_path,
								  std::string &err,
								  std::function<void(int)> progress_cb) {
	FILE *fp = fopen(pe_file.c_str(), "rb");
	if (!fp) {
		err = "Failed to open PE file: " + pe_file;
		return false;
	}

	OffsetFile of{fp, offset, size, 0};
	zlib_filefunc_def zfunc;
	fill_offset_filefunc(&zfunc, &of);

	unzFile uf = unzOpen2(nullptr, &zfunc);
	if (!uf) {
		err = "Failed to open embedded ZIP from PE file.";
		fclose(fp);
		return false;
	}

	// 先统计文件总数
	int total_files = 0;
	if (unzGoToFirstFile(uf) == UNZ_OK) {
		do {
			char filename[260];
			unz_file_info fi;
			if (unzGetCurrentFileInfo(uf, &fi, filename, sizeof(filename), NULL,
									  0, NULL, 0) == UNZ_OK) {
				if (!(filename[strlen(filename) - 1] == '/' ||
					  filename[strlen(filename) - 1] == '\\'))
					total_files++;
			}
		} while (unzGoToNextFile(uf) == UNZ_OK);
	}

	// 解压
	unzGoToFirstFile(uf);
	int current_file = 0;
	do {
		char filename_inzip[260] = {0};
		unz_file_info file_info;
		if (unzGetCurrentFileInfo(uf, &file_info, filename_inzip,
								  sizeof(filename_inzip), NULL, 0, NULL,
								  0) != UNZ_OK) {
			err = "Failed to get file info inside ZIP.";
			break;
		}

		bool is_utf8 = (file_info.flag & 0x800) != 0;

		if (unzOpenCurrentFile(uf) != UNZ_OK) {
			err = "Failed to open file in ZIP: " + std::string(filename_inzip);
			continue;
		}

		// 拼接路径
		std::string out_path = install_path + "\\" + filename_inzip;
		if (is_utf8) {
			out_path = utils::UTF8ToLocal(out_path);
		}

		// 创建目录
		size_t last_slash = out_path.find_last_of("/\\");
		if (last_slash != std::string::npos) {
			create_directories(out_path.substr(0, last_slash));
		}

		// 跳过目录
		if (filename_inzip[strlen(filename_inzip) - 1] == '/' ||
			filename_inzip[strlen(filename_inzip) - 1] == '\\') {
			unzCloseCurrentFile(uf);
			continue;
		}

		FILE *out = fopen(out_path.c_str(), "wb");
		if (!out) {
			err = "Failed to create file: " + out_path;
			unzCloseCurrentFile(uf);
			continue;
		}

		std::vector<char> buf(8192);
		int bytes_read = 0;
		while ((bytes_read = unzReadCurrentFile(uf, buf.data(), buf.size())) >
			   0) {
			fwrite(buf.data(), 1, bytes_read, out);
		}

		fclose(out);
		unzCloseCurrentFile(uf);

		// 更新进度
		++current_file;
		if (progress_cb && total_files > 0) {
			int progress = static_cast<int>((current_file * 80) /
											total_files); // 解压阶段最多到 80
			progress_cb(progress);
		}

	} while (unzGoToNextFile(uf) == UNZ_OK);

	unzClose(uf);
	fclose(fp);
	if (progress_cb)
		progress_cb(80); // 解压完成
	return true;
}