#include "luab.h"
#include <base/base.h>
#include <string>
#include <utils.h>

int lua_del_reg(lua_State *L) {
	std::string key_parent = luaL_checkstring(L, 1);
	std::string key_name = luaL_checkstring(L, 2);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		return luaL_error(L, "del failed, %s not found", key_parent.c_str());
	}
	std::wstring key_name_w = utils::UTF8ToUTF16(key_name);
	utils::DeleteRegistryKey(hkey, key_name_w.c_str());
	return 0;
}

int lua_del_reg_value(lua_State *L) {
	std::string key_parent = luaL_checkstring(L, 1);
	std::string key_name = luaL_checkstring(L, 2);
	std::string value = luaL_checkstring(L, 3);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		return luaL_error(L, "del failed, %s not found", key_parent.c_str());
	}
	std::wstring key_name_w = utils::UTF8ToUTF16(key_name);
	std::wstring value_w = utils::UTF8ToUTF16(value);
	utils::DeleteRegistryValue(hkey, key_name_w.c_str(), value_w.c_str());
	return 0;
}

int lua_set_reg_string(lua_State *L) {
	std::string key_parent = luaL_checkstring(L, 1);
	std::string key_name = luaL_checkstring(L, 2);
	std::string value_name = luaL_checkstring(L, 3);
	std::string value = luaL_checkstring(L, 4);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		return luaL_error(L, "set failed, %s not found", key_parent.c_str());
	}
	std::wstring key_name_w = utils::UTF8ToUTF16(key_name);
	std::wstring value_name_w = utils::UTF8ToUTF16(value_name);
	std::wstring value_w = utils::UTF8ToUTF16(value);
	utils::SetRegistryStringValue(hkey, key_name_w.c_str(),
								  value_name_w.c_str(), value_w.c_str());
	return 0;
}


int lua_set_reg_string_ex(lua_State *L) {
	std::string key_parent = luaL_checkstring(L, 1);
	std::string key_name = luaL_checkstring(L, 2);
	std::string value_name = luaL_checkstring(L, 3);
	std::string value = luaL_checkstring(L, 4);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		return luaL_error(L, "set failed, %s not found", key_parent.c_str());
	}
	std::wstring key_name_w = utils::UTF8ToUTF16(key_name);
	std::wstring value_name_w = utils::UTF8ToUTF16(value_name);
	std::wstring value_w = utils::UTF8ToUTF16(value);
	utils::SetRegistryStringValueEx(hkey, key_name_w.c_str(),
								  value_name_w.c_str(), value_w.c_str());
	return 0;
}

int lua_set_reg_multistring(lua_State *L) {
    // 参数检查
    std::string key_parent = luaL_checkstring(L, 1);
    std::string key_name = luaL_checkstring(L, 2);
    std::string value_name = luaL_checkstring(L, 3);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		return luaL_error(L, "set failed, %s not found", key_parent.c_str());
	}

    // 第4个参数是 table，每个元素是一个字符串
    if (!lua_istable(L, 4)) {
        return luaL_error(L, "value must be a table of strings");
    }

    // 拼接成多字符串，中间 \0，末尾双 \0
    std::vector<std::wstring> values_w;
    size_t count = lua_rawlen(L, 4);
    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, 4, i);
        if (!lua_isstring(L, -1)) {
            return luaL_error(L, "all table elements must be strings");
        }
        std::string s = lua_tostring(L, -1);
        values_w.push_back(utils::UTF8ToUTF16(s));
        lua_pop(L, 1);
    }

    // 拼接成单个 REG_MULTI_SZ buffer
    std::vector<wchar_t> multi_sz;
    for (auto &s : values_w) {
        multi_sz.insert(multi_sz.end(), s.begin(), s.end());
        multi_sz.push_back(L'\0'); // 每个字符串结束
    }
    multi_sz.push_back(L'\0'); // 最后双 \0

    utils::SetRegistryMultiStringValue(
        hkey,
        utils::UTF8ToUTF16(key_name).c_str(),
        utils::UTF8ToUTF16(value_name).c_str(),
        multi_sz.data()
    );

    return 0;
}

int lua_get_reg_string(lua_State *L) {
	std::string key_parent = luaL_checkstring(L, 1);
	std::string key_name = luaL_checkstring(L, 2);
	std::string value_name = luaL_checkstring(L, 3);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		return luaL_error(L, "get failed, %s not found", key_parent.c_str());
	}
	std::wstring key_name_w = utils::UTF8ToUTF16(key_name);
	std::wstring value_name_w = utils::UTF8ToUTF16(value_name);
	std::wstring value_w;
	if (utils::GetRegistryStringValue(hkey, key_name_w.c_str(),
									  value_name_w.c_str(), value_w)) {
		lua_pushstring(L, utils::UTF16ToUTF8(value_w).c_str());
		return 1;
	}
	lua_pushstring(L, "");
	return 1;
}

int lua_get_reg_dword(lua_State *L) {
	std::string key_parent = luaL_checkstring(L, 1);
	std::string key_name = luaL_checkstring(L, 2);
	std::string value_name = luaL_checkstring(L, 3);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		lua_pushinteger(L, 0);
		return 1;
	}
	std::wstring key_name_w = utils::UTF8ToUTF16(key_name);
	std::wstring value_name_w = utils::UTF8ToUTF16(value_name);
	DWORD dwValue = 0;
	utils::GetRegistryDWordValue(hkey, key_name_w.c_str(), value_name_w.c_str(),
								 dwValue);
	lua_pushinteger(L, dwValue);
	return 1;
}

int lua_set_reg_dword(lua_State *L) {
	std::string key_parent = luaL_checkstring(L, 1);
	std::string key_name = luaL_checkstring(L, 2);
	std::string value_name = luaL_checkstring(L, 3);
	int value = luaL_checkinteger(L, 4);

	HKEY hkey = utils::get_hkey(key_parent);
	if (hkey == 0) {
		return luaL_error(L, "set failed, %s not found", key_parent.c_str());
	}
	std::wstring key_name_w = utils::UTF8ToUTF16(key_name);
	std::wstring value_name_w = utils::UTF8ToUTF16(value_name);

	utils::SetRegistryDWORDValue(hkey, key_name_w.c_str(), value_name_w.c_str(),
								 value);
	return 0;
}