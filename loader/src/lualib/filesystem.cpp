#include "luab.h"
#include <base/base.h>
#include <filesystem>
#include <fstream>
#include <hv/hlog.h>
#include <string>
#include <utils/utils.h>

namespace fs = std::filesystem;

int lua_write_file_string(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	const char *str = luaL_checkstring(L, 2);
	BOOL is_append = luaL_optinteger(L, 3, 0);
	try {
		auto mod = std::ios::out;
		if (is_append) {
			mod = std::ios::out | std::ios::app;
		}
		std::fstream fs(path, mod);
		if (fs.is_open()) {
			return luaL_error(L, "create file failed: %s", path);
		}
		fs << str;
		fs.close();
	} catch (const fs::filesystem_error &e) {
		return luaL_error(L, e.what());
	}
	return 0;
}

int lua_cp(lua_State *L) {
	const char *src = luaL_checkstring(L, 1);
	const char *dst = luaL_checkstring(L, 2);
	try {
		fs::copy(src, dst,
				 fs::copy_options::recursive |
					 fs::copy_options::overwrite_existing);
	} catch (const fs::filesystem_error &e) {
		return luaL_error(L, e.what());
	}
	return 0;
}

int lua_mv(lua_State *L) {
	const char *src = luaL_checkstring(L, 1);
	const char *dst = luaL_checkstring(L, 2);
	std::error_code ec;
	fs::copy(src, dst,
			 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
			 ec);
	if (ec) {
		LOGE("%s", utils::LocalToUTF8(ec.message()).c_str());
		return 0;
	}
	fs::remove_all(src, ec);
	if (ec) {
		LOGE("%s", utils::LocalToUTF8(ec.message()).c_str());
	}
	return 0;
}

int lua_mkdir(lua_State *L) {
	const char *src = luaL_checkstring(L, 1);
	std::error_code ec;
	fs::create_directories(src, ec);
	if (ec) {
		LOGE("%s", utils::LocalToUTF8(ec.message()).c_str());
	}
	return 0;
}

int lua_rm(lua_State *L) {
	const char *src = luaL_checkstring(L, 1);
	std::error_code ec;
	fs::remove_all(src, ec);
	if (ec) {
		LOGE("%s", utils::LocalToUTF8(ec.message()).c_str());
	}
	return 0;
}

int lua_exist(lua_State *L) {
	const char *src = luaL_checkstring(L, 1);
	lua_pushboolean(L, fs::exists(src));
	return 1;
}
