#include "luab.h"
#include <hv/hlog.h>
#include <stdio.h>


int lua_log(lua_State *L, int level) {
	int n = lua_gettop(L); // 参数个数
	if (n == 0) {
		luaL_error(L, "log requires at least 1 argument");
		return 0;
	}

	// 获取格式字符串
	const char *fmt = luaL_checkstring(L, 1);

	// 为了格式化参数，可以调用Lua自己的string.format，再传给logi
	// 调用Lua的string.format(fmt, arg2, arg3, ...)
	lua_getglobal(L, "string");
	lua_getfield(L, -1, "format"); // push string.format函数
	lua_pushstring(L, fmt);

	// 把剩余参数复制到栈顶（arg2..argN）
	for (int i = 2; i <= n; i++) {
		lua_pushvalue(L, i);
	}

	// 调用 string.format(fmt, args...)，返回格式化后的字符串
	if (lua_pcall(L, n, 1, 0) != LUA_OK) {
		// 报错
		const char *err = lua_tostring(L, -1);
		luaL_error(L, "string.format error: %s", err);
		return 0;
	}

	// 获取格式化字符串
	const char *formatted = lua_tostring(L, -1);

	// 调用logi输出
	switch (level) {
	case LOG_LEVEL_DEBUG:
		LOGD("%s", formatted);
		return 0;
	case LOG_LEVEL_INFO:
		LOGI("%s", formatted);
		return 0;
	case LOG_LEVEL_WARN:
		LOGW("%s", formatted);
		return 0;
	case LOG_LEVEL_ERROR:
		LOGE("%s", formatted);
		return 0;
	case LOG_LEVEL_FATAL:
		LOGF("%s", formatted);
		return 0;
	default:
		return 0;
	}
	return 0; // 无返回值给Lua
}


int lua_logd(lua_State *L) {
	return lua_log(L, LOG_LEVEL_DEBUG);
}


int lua_logi(lua_State *L) {
	return lua_log(L, LOG_LEVEL_INFO);
}


int lua_logw(lua_State *L) {
	return lua_log(L, LOG_LEVEL_WARN);
}


int lua_loge(lua_State *L) {
	return lua_log(L, LOG_LEVEL_ERROR);
}


int lua_logf(lua_State *L) {
	return lua_log(L, LOG_LEVEL_FATAL);
}
