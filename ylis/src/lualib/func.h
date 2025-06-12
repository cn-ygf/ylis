#pragma once

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <string>
#include <map>
#include <cstdint>

// 编译状态数据
struct ylis_build_state {
    std::map<std::string, std::string> file;        // add_file 数据
    std::map<std::string, std::string> res;         // add_res  数据
    std::map<std::string, std::string> opt_str;     // opt 字符串类型数据
    std::map<std::string, int64_t> opt_int;        // opt 整数型数据
    std::map<std::string, double> opt_float;        // opt 浮点型数据
    std::string icon_path;
};

void register_functions(lua_State* L);