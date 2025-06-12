#include "build.h"
#include "lualib/func.h"
#include <base/base.h>
#include <condition_variable>
#include <csignal>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <hv/base64.h>
#include <hv/hlog.h>
#include <hv/json.hpp>
#include <iostream>
#include <utils/utils.h>
#include <utils/utils_res.h>
#include <utils/utils_zip.h>
#include <windows.h>

#define IDR_THEME 1001
#define IDR_THEME_OPT 1002
#define IDR_THEME_LUA 1003

static char g_skin_bin_data[] = {
#include "skin.bin.h"
};
static char g_skin_global_data[] = {
#include "global.xml.h"
};
static char g_skin_main_data[] = {
#include "main.xml.h"
};
static char g_skin_dlg_data[] = {
#include "dlg.xml.h"
};
static char g_skin_uninstall_data[] = {
#include "uninstall.xml.h"
};
static char g_loader_data[] = {
#include "loader.bin.h"
};

std::condition_variable cv;
std::mutex cv_m;

void my_logger(int loglevel, const char *buf, int len) {
	stdout_logger(loglevel, buf, len);
}

void signal_handler(int signal) {
	if (signal == SIGINT || signal == SIGTERM) {
		LOGI("received exit signal...");
		cv.notify_all();
	}
}

void usage() {
	std::cout << "使用方法:\n";
	std::cout << "\tylis.exe <LUA脚本文件>\n";
}

// extern std::string g_lua_path;
std::string g_lua_path;

int main(int argc, char *argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	if (argc < 2) {
		usage();
		return 0;
	}
	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);
	hlog_set_level(LOG_LEVEL_INFO);
	// hlog_set_format("%y-%m-%d %H:%M:%S.%z %L %s");
	hlog_set_handler(my_logger);
	logger_enable_color(hlog, 1);

	std::string lua_path = utils::MakeAbsolutePath(argv[1]);
	std::wstring lua_dir;
	nbase::FilePathApartDirectory(utils::LocalToUTF16(lua_path), lua_dir);
	g_lua_path = utils::UTF16ToUTF8(lua_dir);
	LOGI("lua_path %s", g_lua_path.c_str());

	// 创建lua虚拟机
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	register_functions(L);
	// 设置状态
	std::unique_ptr<ylis_build_state> state_ptr =
		std::make_unique<ylis_build_state>();
	lua_pushlightuserdata(L, state_ptr.get());
	lua_setfield(L, LUA_REGISTRYINDEX, "ylis_build_state");

	if (luaL_dofile(L, lua_path.c_str()) != LUA_OK) {
		LOGE("LUA脚本运行错误： %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);
		return 1;
	}
	// 调用 Lua 函数 build()
	lua_getglobal(L, "build");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			LOGE("调用 build 函数出现错误： %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			lua_close(L);
			return 1;
		}
	} else {
		LOGE("没有找到 build 函数");
		lua_pop(L, 1);
		lua_close(L);
		return 1;
	}
	std::error_code ec;

	// 创建工作目录
	std::string work_dir = g_lua_path + ".ylis";
	if (std::filesystem::exists(work_dir)) {
		std::filesystem::remove_all(work_dir, ec);
	}
	std::filesystem::create_directories(work_dir, ec);
	// 打包安装目录为zip
	LOGI("正在打包安装文件");
	std::string app_zip = work_dir + "\\" + "app.zip";
	std::string err;

	if (!build_install_with_zip(state_ptr.get(), app_zip, err)) {
		LOGE("打包安装文件失败：%s", err.c_str());
		lua_close(L);
		return 1;
	}

	// 打包皮肤文件
	LOGI("正在打包皮肤资源");
	std::string skin_zip = work_dir + "\\" + "skin.zip";
	err.clear();

	std::string skin_buffer(reinterpret_cast<const char *>(g_skin_bin_data),
							sizeof(g_skin_bin_data));
	std::string skin_global_buffer = g_skin_global_data;
	std::string skin_main_buffer = g_skin_main_data;
	std::string skin_dlg_buffer = g_skin_dlg_data;
	std::string skin_uninstall_buffer = g_skin_uninstall_data;
	// std::string skin_global_buffer(reinterpret_cast<const
	// char*>(g_skin_global_data), sizeof(g_skin_global_data)); std::string
	// skin_main_buffer(reinterpret_cast<const char*>(g_skin_main_data),
	// sizeof(g_skin_main_data));

	// 替换参数
	for (auto &item : state_ptr->opt_str) {
		std::string key = nbase::StringPrintf("{{%s}}", item.first.c_str());
		LOGI("%s->%s", key.c_str(), item.second.c_str());
		nbase::StringReplaceAll(key, item.second, skin_global_buffer);
		nbase::StringReplaceAll(key, item.second, skin_main_buffer);
		nbase::StringReplaceAll(key, item.second, skin_uninstall_buffer);
	}

	std::map<std::string, std::string> update_list;
	update_list["resources/themes/default/global.xml"] = skin_global_buffer;
	update_list["resources/themes/default/ylis/main.xml"] = skin_main_buffer;
	update_list["resources/themes/default/ylis/dlg.xml"] = skin_dlg_buffer;
	update_list["resources/themes/default/ylis/uninstall.xml"] =
		skin_uninstall_buffer;
	for (auto &item : state_ptr->res) {
		std::string key =
			fmt::format("resources/themes/default/ylis/{}", item.second);
		std::string key_local = utils::UTF8ToLocal(key);
		std::wstring file_name = utils::UTF8ToUTF16(item.first);
		std::ifstream fs(file_name, std::ios::in | std::ios::binary);
		if (!fs) {
			LOGW("open %s failed", item.first.c_str());
			continue;
		}
		fs.seekg(0, std::ios::end);
		std::streamsize size = fs.tellg();
		fs.seekg(0, std::ios::beg);
		std::string value;
		value.resize(size);
		fs.read(value.data(), size);
		fs.close();
		update_list[key_local] = value;
	}
	if (!utils::skin_resource_update(skin_buffer, update_list, skin_zip, err)) {
		LOGE("打包皮肤资源失败：%s", err.c_str());
		lua_close(L);
		return 1;
	}

	// 生成PE文件
	LOGI("正在编译可执行文件");
	std::string app_bin = work_dir + "\\" + "setup.bin";
	std::ofstream outfs(app_bin, std::ios::binary | std::ios::out);
	if (!outfs) {
		LOGE("创建文件 %s 失败", app_bin.c_str());
		lua_close(L);
		return 1;
	}
	outfs.write(g_loader_data, sizeof(g_loader_data));
	outfs.close();
	// 写入皮肤资源
	err.clear();
	if (!utils::update_exe_resource_with_file(app_bin, skin_zip, IDR_THEME,
											  "THEME", err)) {
		LOGE("PE文件写入资源 %s 失败", app_bin.c_str());
		lua_close(L);
		return 1;
	}
	// 写入ico
	err.clear();
	if (!state_ptr->icon_path.empty()) {
		if (!utils::update_exe_icon(app_bin, state_ptr->icon_path, err)) {
			LOGE("PE文件写入图标 %s 失败", app_bin.c_str());
			lua_close(L);
			return 1;
		}
	}

	// 写入version
	if (state_ptr->opt_str.count("version") > 0) {
		std::string product;
		std::string company;
		std::string err;
		if (state_ptr->opt_str.count("app_name") > 0) {
			product = state_ptr->opt_str["app_name"];
		}
		if (state_ptr->opt_str.count("publisher") > 0) {
			company = state_ptr->opt_str["publisher"];
		}
		if (!write_version_res_data(app_bin, state_ptr->opt_str["version"],
									product, company, err)) {
			LOGW("PE文件写入版本信息 %s 失败", app_bin.c_str());
		}
	}

	// 写入参数和脚本
	nlohmann::json opt;
	for (auto &item : state_ptr->opt_str) {
		opt[item.first] = item.second;
	}
	for (auto &item : state_ptr->opt_int) {
		opt[item.first] = item.second;
	}
	for (auto &item : state_ptr->opt_float) {
		opt[item.first] = item.second;
	}
	std::string opt_json_str = opt.dump();
	std::string opt_base64 = hv::Base64Encode(
		(const uint8_t *)opt_json_str.data(), opt_json_str.size());

	std::string lua_buffer;
	std::ifstream luafs(lua_path);
	if (!luafs) {
		LOGE("PE文件写脚本文件 %s 失败", lua_path.c_str());
		lua_close(L);
		return 1;
	}
	luafs.seekg(0, std::ios::end);
	std::streamsize lua_size = luafs.tellg();
	luafs.seekg(0, std::ios::beg);
	lua_buffer.resize(lua_size);
	luafs.read(lua_buffer.data(), lua_size);
	luafs.close();

	std::string lua_base64 =
		hv::Base64Encode((const uint8_t *)lua_buffer.data(), lua_buffer.size());

	err.clear();
	if (!utils::update_exe_resource_with_mem(app_bin, lua_base64.data(),
											 lua_base64.size(), IDR_THEME_LUA,
											 "THEME", err)) {
		LOGE("PE文件写入脚本 %s 失败", app_bin.c_str());
		lua_close(L);
		return 1;
	}
	err.clear();
	if (!utils::update_exe_resource_with_mem(app_bin, opt_base64.data(),
											 opt_base64.size(), IDR_THEME_OPT,
											 "THEME", err)) {
		LOGE("PE文件写入参数 %s 失败", app_bin.c_str());
		lua_close(L);
		return 1;
	}
	// 写入安装数据
	err.clear();
	if (!writer_install_data(app_bin, app_zip, err)) {
		LOGE("PE文件写入安装数据 %s 失败", app_bin.c_str());
		lua_close(L);
		return 1;
	}

	std::string outfilename =
		fmt::format("{0}.exe", state_ptr->opt_str["app_name"]);
	if (state_ptr->opt_str.count("out")) {
		outfilename = state_ptr->opt_str["out"];
	}
	std::string outfile = utils::MakeAbsolutePath(outfilename, g_lua_path);
	lua_close(L);
	if (std::filesystem::exists(outfile)) {
		std::filesystem::remove(outfile, ec);
		if (ec) {
			LOGE("%s 被占用，无法写入", outfile.c_str());
			return 0;
		}
	}
	std::filesystem::copy(app_bin, outfile, ec);
	std::filesystem::remove_all(work_dir, ec);
	LOGI("编译成功：%s", outfile.c_str());
	return 0;
}
