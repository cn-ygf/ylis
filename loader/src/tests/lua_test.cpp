#include "../lualib/luab.h"
#include <iostream>
#include <memory>
#include <hv/hlog.h>

static char g_lualib_data[] = {
    #include "lualib.lua.h"
};

void my_logger(int loglevel, const char *buf, int len) {
	stdout_logger(loglevel, buf, len);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "lua_test.exe xxx.lua\n";
		return 0;
	}
	hlog_set_level(LOG_LEVEL_INFO);
	// hlog_set_format("%y-%m-%d %H:%M:%S.%z %L %s");
	hlog_set_handler(my_logger);
	logger_enable_color(hlog, 1);

	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	// 注册cpp函数
	register_functions(L);
	// 注册lua函数
	if(luaL_dostring(L, g_lualib_data) != LUA_OK) {
		std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
		lua_close(L);
		return 1;
	}

	// 设置状态
	std::unique_ptr<ylis_state> state_ptr = std::make_unique<ylis_state>();
	lua_pushlightuserdata(L, state_ptr.get());
	lua_setfield(L, LUA_REGISTRYINDEX, "ylis_state");
	state_ptr->state["guid"] = "{41692E8C-DBBC-4373-87A7-98883E9BFD2B}";
	state_ptr->state["app_name"] = "小小小记事本";
	state_ptr->state["main_bin"] = "notepad.exe";
	state_ptr->state["install_dir"] = "C:\\Windows";
	state_ptr->state_int["hide"] = 0;  // bool用字符串表示
	state_ptr->state["title"] = "小小小记事本安装向导";
	state_ptr->state["master_color"] = "#ff07c160";
	state_ptr->state["bg_color"] = "#fff7f7f7";
	state_ptr->state["service_agreement_url"] = "https://www.baidu.com";
	state_ptr->state["service_agreement_text"] = "服务协议";
	state_ptr->state["install_button_text"] = "立即安装";
	state_ptr->state["logo"] = "logo.png";
	state_ptr->state["icon"] = "logo.icon";
	state_ptr->state["manifest"] = "app.manifest";
	state_ptr->state["version"] = "1.0.1";
	state_ptr->state["publisher"] = "星河软件公司";

	if (luaL_dofile(L, argv[1]) != LUA_OK) {
		std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
		lua_close(L);
		return 1;
	}
	// 调用 Lua 函数 init()
	lua_getglobal(L, "init");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			std::cerr << "Lua error in init(): " << lua_tostring(L, -1)
					  << std::endl;
			lua_pop(L, 1);
		}
	} else {
		std::cerr << "Lua function 'init' not found!" << std::endl;
		lua_pop(L, 1);
	}

	lua_close(L);
	return 0;
}