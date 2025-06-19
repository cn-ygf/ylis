#include "build.h"
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <utils/utils.h>

// 压缩安装目录所需文件
bool build_install_with_zip(ylis_build_state *p, const std::string &out,
							std::string &err) {
	zipFile zf = zipOpen(out.c_str(), 0);
	if (!zf) {
		err = "Failed to create output ZIP file: " + out;
		return false;
	}

	for (auto &item : p->file) {
        std::wstring file_name = utils::UTF8ToUTF16(item.first);
		if (!std::filesystem::exists(file_name)) {
			zipClose(zf, NULL);
			err = fmt::format("{} not found", item.first);
			return false;
		}

        // 转换为本地编码
        std::string zip_file_name = utils::UTF8ToLocal(item.second);

		if (zipOpenNewFileInZip(zf, zip_file_name.c_str(), NULL, NULL, 0, NULL, 0,
								NULL, Z_DEFLATED,
								Z_DEFAULT_COMPRESSION) != ZIP_OK) {
			err = "Failed to create new file in ZIP: " + item.second;
			zipClose(zf, NULL);
			return false;
		}

		std::ifstream fs(file_name, std::ios::in | std::ios::binary);
		if (!fs) {
			zipCloseFileInZip(zf);
			zipClose(zf, NULL);
			err = fmt::format("open {} failed", item.first);
			return false;
		}

		std::vector<char> buffer(4096);
		while (fs.read(buffer.data(), buffer.size()) || fs.gcount() > 0) {
			std::streamsize read_bytes = fs.gcount();
			if (zipWriteInFileInZip(zf, buffer.data(),
									static_cast<unsigned int>(read_bytes)) !=
				ZIP_OK) {
				fs.close();
				zipCloseFileInZip(zf);
				zipClose(zf, NULL);
				err = "Failed to write file to ZIP: " + item.first;
				return false;
			}
		}

		fs.close();
		zipCloseFileInZip(zf);
	}

	zipClose(zf, NULL);
	return true;
}

bool writer_install_data(const std::string &pe_file,
						 const std::string &install_data_file,
						 std::string &err) {
	std::ifstream pe_in(pe_file, std::ios::binary | std::ios::ate);
	std::ifstream data_in(install_data_file, std::ios::binary | std::ios::ate);
	if (!pe_in || !data_in) {
		err = "Failed to open PE or install data file.";
		return false;
	}

	std::streamsize pe_size = pe_in.tellg();
	std::streamsize data_size = data_in.tellg();
	pe_in.seekg(0);
	data_in.seekg(0);

	std::vector<char> data_content(data_size);
	data_in.read(data_content.data(), data_size);

	unsigned char md5[16];
    std::wstring install_data_file_w = utils::LocalToUTF16(install_data_file);
	if (!utils::CalculateFileMD5(install_data_file_w, md5)) {
		err = "Failed to calculate MD5 from install_data_file.";
		return false;
	}

	ylis_header header{};
	std::memcpy(header.magic, "YLIS", 4);
	header.m = 0x09;
	header.j = 0x03;
	header.offset = static_cast<uint64_t>(pe_size + sizeof(ylis_header));
	header.size = static_cast<uint64_t>(data_size);
	std::memcpy(header.hash, md5, 16);

	std::ofstream pe_out(pe_file, std::ios::binary | std::ios::app);
	if (!pe_out) {
		err = "Failed to open PE file for writing.";
		return false;
	}

	pe_out.write(reinterpret_cast<const char *>(&header), sizeof(header));
	pe_out.write(data_content.data(), data_size);

	return true;
}

// Helper: WORD对齐长度
inline size_t AlignToDword(size_t size) { return (size + 3) & ~3; }

void AlignToDWORD(std::vector<BYTE>& data) {
    while (data.size() % 4) {
        data.push_back(0);
    }
}

size_t WriteUnicodeString(std::vector<BYTE>& data, const std::wstring& str) {
    size_t len = (str.length() + 1) * sizeof(wchar_t);
    const BYTE* p = reinterpret_cast<const BYTE*>(str.c_str());
    data.insert(data.end(), p, p + len);
    return len;
}

void WriteWORD(std::vector<BYTE>& data, WORD value) {
    data.insert(data.end(), (BYTE*)&value, (BYTE*)&value + sizeof(WORD));
}

void WriteDWORD(std::vector<BYTE>& data, DWORD value) {
    data.insert(data.end(), (BYTE*)&value, (BYTE*)&value + sizeof(DWORD));
}

void WriteFixedFileInfo(std::vector<BYTE>& data, WORD major, WORD minor, WORD patch, WORD revision) {
    VS_FIXEDFILEINFO info = {};
    info.dwSignature = 0xFEEF04BD;
    info.dwStrucVersion = 0x00010000;
    info.dwFileVersionMS = MAKELONG(minor, major);
    info.dwFileVersionLS = MAKELONG(revision, patch);
    info.dwProductVersionMS = info.dwFileVersionMS;
    info.dwProductVersionLS = info.dwFileVersionLS;
    info.dwFileFlagsMask = 0x3F;
    info.dwFileFlags = 0;
    info.dwFileOS = 0x40004;
    info.dwFileType = 0x1;
    info.dwFileSubtype = 0;
    info.dwFileDateMS = 0;
    info.dwFileDateLS = 0;

    data.insert(data.end(), (BYTE*)&info, (BYTE*)&info + sizeof(info));
}

std::vector<BYTE> BuildVersionResource(WORD major, WORD minor, WORD patch, WORD revision,
                                       const std::map<std::wstring, std::wstring>& strings) {
    std::vector<BYTE> data;

    size_t root_start = data.size();
    WriteWORD(data, 0); // wLength (patch later)
    WriteWORD(data, sizeof(VS_FIXEDFILEINFO));
    WriteWORD(data, 0); // binary
    WriteUnicodeString(data, L"VS_VERSION_INFO");
    AlignToDWORD(data);

    WriteFixedFileInfo(data, major, minor, patch, revision);

    // ---- StringFileInfo block ----
    size_t sfi_start = data.size();
    WriteWORD(data, 0); // wLength
    WriteWORD(data, 0);
    WriteWORD(data, 1);
    WriteUnicodeString(data, L"StringFileInfo");
    AlignToDWORD(data);

    // ---- StringTable block ----
    size_t st_start = data.size();
    WriteWORD(data, 0);
    WriteWORD(data, 0);
    WriteWORD(data, 1);
    WriteUnicodeString(data, L"040904B0");
    AlignToDWORD(data);

    for (const auto& kv : strings) {
        size_t item_start = data.size();
        WriteWORD(data, 0);
        WriteWORD(data, (WORD)(kv.second.length() + 1));
        WriteWORD(data, 1);
        WriteUnicodeString(data, kv.first);
        AlignToDWORD(data);
        WriteUnicodeString(data, kv.second);
        AlignToDWORD(data);
        WORD item_len = (WORD)(data.size() - item_start);
        memcpy(&data[item_start], &item_len, sizeof(WORD));
    }

    WORD st_len = (WORD)(data.size() - st_start);
    memcpy(&data[st_start], &st_len, sizeof(WORD));

    WORD sfi_len = (WORD)(data.size() - sfi_start);
    memcpy(&data[sfi_start], &sfi_len, sizeof(WORD));

    // ---- VarFileInfo ----
    size_t vfi_start = data.size();
    WriteWORD(data, 0);
    WriteWORD(data, 0);
    WriteWORD(data, 1);
    WriteUnicodeString(data, L"VarFileInfo");
    AlignToDWORD(data);

    size_t var_start = data.size();
    WriteWORD(data, 0);
    WriteWORD(data, sizeof(DWORD));
    WriteWORD(data, 0);
    WriteUnicodeString(data, L"Translation");
    AlignToDWORD(data);
    WriteDWORD(data, 0x040904B0);
    WORD var_len = (WORD)(data.size() - var_start);
    memcpy(&data[var_start], &var_len, sizeof(WORD));

    WORD vfi_len = (WORD)(data.size() - vfi_start);
    memcpy(&data[vfi_start], &vfi_len, sizeof(WORD));

    WORD root_len = (WORD)(data.size() - root_start);
    memcpy(&data[root_start], &root_len, sizeof(WORD));

    return data;
}

bool write_version_res_data(const std::string& pe_file,
                            const std::string& version,
                            const std::string& product,
                            const std::string& company,
                            std::string& err) {
    std::wstring version_w = utils::UTF8ToUTF16(version);
    std::wstring product_w = utils::UTF8ToUTF16(product);
    std::wstring company_w = utils::UTF8ToUTF16(company);

    WORD major = 0, minor = 0, patch = 0, revision = 0;
    sscanf(version.c_str(), "%hu.%hu.%hu.%hu", &major, &minor, &patch, &revision);

    std::map<std::wstring, std::wstring> strings = {
        {L"CompanyName", company_w},
        {L"FileDescription", product_w},
        {L"FileVersion", version_w},
        {L"InternalName", product_w},
        {L"LegalCopyright", L"Copyright © 2025"},
        {L"OriginalFilename", product_w + L".exe"},
        {L"ProductName", product_w},
        {L"ProductVersion", version_w}
    };

    std::vector<BYTE> res;
    try {
        res = BuildVersionResource(major, minor, patch, revision, strings);
    } catch (...) {
        err = "BuildVersionResource failed";
        return false;
    }

    std::wstring pe_wfile = utils::UTF8ToUTF16(pe_file);
    HANDLE hUpdate = BeginUpdateResourceW(pe_wfile.c_str(), FALSE);
    if (!hUpdate) {
        err = "BeginUpdateResourceW failed";
        return false;
    }

    if (!UpdateResourceW(hUpdate, RT_VERSION, MAKEINTRESOURCEW(1),
                         MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
                         res.data(), (DWORD)res.size())) {
        EndUpdateResourceW(hUpdate, TRUE);
        err = "UpdateResourceW failed";
        return false;
    }

    if (!EndUpdateResourceW(hUpdate, FALSE)) {
        err = "EndUpdateResourceW failed";
        return false;
    }

    return true;
}