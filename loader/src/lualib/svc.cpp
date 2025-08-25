#include "luab.h"
#include <utils/utils.h>

int lua_create_service(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);
	std::string display_name = luaL_checkstring(L, 2);
	std::string description = luaL_checkstring(L, 3);
	std::string bin_path = luaL_checkstring(L, 4);
	int typ = luaL_checkinteger(L, 5); // 1 = SERVICE_KERNEL_DRIVER(内核驱动服务)  2 SERVICE_FILE_SYSTEM_DRIVER(文件驱动服务) 16 SERVICE_WIN32_OWN_PROCESS(普通服务)
	int start_mode =
		luaL_checkinteger(L, 6); // 0 = 自动, 1 = 延迟自动, 2 = 手动, 3 = 禁止 4 = boot

	std::string err;
	if (utils::create_service(name, display_name, description, bin_path, typ,
							  start_mode, err)) {
		return 0;
	}
	return luaL_error(L, "create_service failed: %s", err.c_str());
}

int lua_change_service_failure_restart(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);
	int delay =
		luaL_checkinteger(L, 2); // 毫秒

	std::string err;
	if (utils::change_service_failure_restart(name, delay, err)) {
		return 0;
	}
	return luaL_error(L, "change_service_failure_restart failed: %s", err.c_str());
}

int lua_change_service_start_mode(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);
	int start_mode =
		luaL_checkinteger(L, 2); // 0 = 自动, 1 = 延迟自动, 2 = 手动, 3 = 禁止, 4 = boot 引导启动（Boot Start）	系统加载引导程序时	内核驱动（boot-start driver)

	std::string err;
	if (utils::change_service_start_mode(name, start_mode, err)) {
		return 0;
	}
	return luaL_error(L, "change_service_start_mode failed: %s", err.c_str());
}

int lua_start_service(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);

	std::string err;
	if (utils::start_service(name, err)) {
		return 0;
	}
	return luaL_error(L, "start_service failed: %s", err.c_str());
}

int lua_stop_service(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);

	std::string err;
	if (utils::stop_service(name, err)) {
		return 0;
	}
	return luaL_error(L, "stop_service failed: %s", err.c_str());
}

int lua_query_service_status(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);

	std::string err;
	int status = 0;
	if (utils::query_service_status(name, status, err)) {
		lua_pushinteger(L, status);
		return 1;
	}
	return luaL_error(L, "query_service_status failed: %s", err.c_str());
}

int lua_service_status_to_string(lua_State *L) {
	int code = luaL_checkinteger(L, 1);
	const char *s = "unknown";
	switch (code) {
	case SERVICE_STOPPED:
		s = "stopped";
		break;
	case SERVICE_START_PENDING:
		s = "starting";
		break;
	case SERVICE_RUNNING:
		s = "running";
		break;
	case SERVICE_STOP_PENDING:
		s = "stopping";
		break;
	case SERVICE_PAUSED:
		s = "paused";
		break;
	case 0xff:
		s = "not found";
		break;
		// ...其他状态
	}
	lua_pushstring(L, s);
	return 1;
}

int lua_delete_service(lua_State *L) {
	std::string name = luaL_checkstring(L, 1);

	std::string err;
	if (utils::delete_service(name, err)) {
		return 0;
	}
	return luaL_error(L, "delete_service failed: %s", err.c_str());
}
