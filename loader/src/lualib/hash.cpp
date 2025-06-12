#include "luab.h"
#include <base/base.h>
#include <string>
#include <utils/utils.h>
#include <hv/base64.h>

int lua_sha256(lua_State *L) {
	std::string str = luaL_checkstring(L, 1);
	std::string hex = utils::CalculateSHA256(str);
    lua_pushstring(L, hex.c_str());
	return 1;
}

int lua_md5(lua_State *L) {
	std::string str = luaL_checkstring(L, 1);
	std::string hex = utils::CalculateMD5((const uint8_t*)str.data(), str.size());
    lua_pushstring(L, hex.c_str());
	return 1;
}

int lua_md5_file(lua_State *L) {
	std::string filename = luaL_checkstring(L, 1);
	std::string hex = utils::CalculateFileMD5(filename);
    lua_pushstring(L, hex.c_str());
	return 1;
}

int lua_base64_encode(lua_State *L) {
	size_t buffer_size;
	const char* buffer = luaL_checklstring(L, 1, &buffer_size);
	if (buffer_size < 1 || !buffer) {
		return luaL_error(L, "data is nil");
	}
	std::string base64_str = hv::Base64Encode((const uint8_t*)buffer, buffer_size);
	lua_pushstring(L, base64_str.c_str());
	return 1;
}

int lua_base64_decode(lua_State *L) {
	const char* base64_str = luaL_checkstring(L, 1);
	std::string buffer = hv::Base64Decode(base64_str);
	lua_pushlstring(L, buffer.c_str(), buffer.size());
	return 1;
}