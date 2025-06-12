#include <fstream>
#include <iostream>
#include <map>
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <sstream>
#include <string>

namespace utils {
// 内存文件结构
typedef struct {
	const unsigned char *buffer;
	size_t size;
	size_t position;
} MemoryFile;

static voidpf ZCALLBACK mem_open_file(voidpf opaque, const char *filename,
									  int mode) {
	return opaque;
}

static uLong ZCALLBACK mem_read_file(voidpf opaque, voidpf stream, void *buf,
									 uLong size) {
	MemoryFile *mf = (MemoryFile *)stream;
	if (mf->position + size > mf->size)
		size = mf->size - mf->position;
	memcpy(buf, mf->buffer + mf->position, size);
	mf->position += size;
	return size;
}

static long ZCALLBACK mem_tell_file(voidpf opaque, voidpf stream) {
	MemoryFile *mf = (MemoryFile *)stream;
	return (long)mf->position;
}

static long ZCALLBACK mem_seek_file(voidpf opaque, voidpf stream, uLong offset,
									int origin) {
	MemoryFile *mf = (MemoryFile *)stream;
	size_t new_pos = 0;

	switch (origin) {
	case ZLIB_FILEFUNC_SEEK_CUR:
		new_pos = mf->position + offset;
		break;
	case ZLIB_FILEFUNC_SEEK_END:
		new_pos = mf->size + offset;
		break;
	case ZLIB_FILEFUNC_SEEK_SET:
		new_pos = offset;
		break;
	default:
		return -1;
	}

	if (new_pos > mf->size)
		return -1;

	mf->position = new_pos;
	return 0;
}

static int ZCALLBACK mem_close_file(voidpf opaque, voidpf stream) {
	return 0; // 不需要释放
}

static int ZCALLBACK mem_error_file(voidpf opaque, voidpf stream) { return 0; }

void fill_memory_filefunc(zlib_filefunc_def *pzlib_filefunc_def,
						  MemoryFile *mf) {
	pzlib_filefunc_def->zopen_file = mem_open_file;
	pzlib_filefunc_def->zread_file = mem_read_file;
	pzlib_filefunc_def->ztell_file = mem_tell_file;
	pzlib_filefunc_def->zseek_file = mem_seek_file;
	pzlib_filefunc_def->zclose_file = mem_close_file;
	pzlib_filefunc_def->zerror_file = mem_error_file;
	pzlib_filefunc_def->opaque = mf;
}

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
						  const std::string &outfile, std::string &err) {
	MemoryFile mf;
	mf.buffer = (const unsigned char *)buffer.data();
	mf.size = buffer.size();
	mf.position = 0;
	zlib_filefunc_def mem_filefunc;
	fill_memory_filefunc(&mem_filefunc, &mf);

	unzFile uf = unzOpen2(nullptr, &mem_filefunc);
	if (!uf) {
		err = "Failed to open input ZIP from memory.";
		return false;
	}

	zipFile zf = zipOpen(outfile.c_str(), 0);
	if (!zf) {
		unzClose(uf);
		err = "Failed to create output ZIP file: " + outfile;
		return false;
	}

	if (unzGoToFirstFile(uf) != UNZ_OK) {
		unzClose(uf);
		zipClose(zf, NULL);
		err = "Failed to locate first file in input ZIP.";
		return false;
	}

	do {
		char filename_inzip[256] = {0};
		unz_file_info file_info;
		if (unzGetCurrentFileInfo(uf, &file_info, filename_inzip,
								  sizeof(filename_inzip), NULL, 0, NULL,
								  0) != UNZ_OK) {
			err = "Failed to retrieve file info from input ZIP.";
			break;
		}

		if (update_list.find(filename_inzip) != update_list.end()) {
			continue; // Skip files that will be replaced
		}

		if (unzOpenCurrentFile(uf) != UNZ_OK) {
			err = std::string("Failed to open file in input ZIP: ") +
				  filename_inzip;
			continue;
		}

		std::string buf;
		buf.resize(file_info.uncompressed_size);
		int read_size = unzReadCurrentFile(uf, (void *)buf.data(),
										   file_info.uncompressed_size);
		if (read_size < 0) {
			err = std::string("Failed to read file in input ZIP: ") +
				  filename_inzip;
			unzCloseCurrentFile(uf);
			continue;
		}

		if (zipOpenNewFileInZip(zf, filename_inzip, NULL, NULL, 0, NULL, 0,
								NULL, Z_DEFLATED,
								Z_DEFAULT_COMPRESSION) != ZIP_OK) {
			err = std::string("Failed to open file in output ZIP: ") +
				  filename_inzip;
			unzCloseCurrentFile(uf);
			continue;
		}

		if (zipWriteInFileInZip(zf, buf.data(), read_size) != ZIP_OK) {
			err = std::string("Failed to write file to output ZIP: ") +
				  filename_inzip;
		}

		zipCloseFileInZip(zf);
		unzCloseCurrentFile(uf);
	} while (unzGoToNextFile(uf) == UNZ_OK);

	// Add or replace files
	for (const auto &item : update_list) {
		if (zipOpenNewFileInZip(zf, item.first.c_str(), NULL, NULL, 0, NULL, 0,
								NULL, Z_DEFLATED,
								Z_DEFAULT_COMPRESSION) != ZIP_OK) {
			err = "Failed to create new file in ZIP: " + item.first;
			continue;
		}

		if (zipWriteInFileInZip(zf, item.second.c_str(), item.second.size()) !=
			ZIP_OK) {
			err = "Failed to write replacement file to ZIP: " + item.first;
		}

		zipCloseFileInZip(zf);
	}

	unzClose(uf);
	zipClose(zf, NULL);

	return true;
}
} // namespace utils