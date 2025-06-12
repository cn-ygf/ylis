#include "func.h"
#include <hv/hlog.h>
#include <base/base.h>
#include <tabulate/table.hpp>
#include <utils/utils.h>

extern std::string g_lua_path;

// 获取build状态数据指针
ylis_build_state* lua_get_build_state(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "ylis_build_state");
    void* p = lua_touserdata(L, -1);
    if (p == nullptr) {
        return nullptr;
    }
    ylis_build_state *p_state = (ylis_build_state*)p;
    return p_state;
}

// add_file(src,dst) 添加文件到打包队列
// src 本地文件路径
// dst 目标文件路径
int lua_add_file(lua_State *L) {
	std::string src = luaL_checkstring(L, 1);
    std::string dst = luaL_checkstring(L, 2);
    if (src.empty() || dst.empty()) {
        return luaL_error(L, "add_file 源文件路径和目标路径不能为空");
    }
    ylis_build_state *p = lua_get_build_state(L);
    if (p == nullptr) {
        return luaL_error(L, "get_build_state failed");
    }
    src = utils::MakeAbsolutePath(src, g_lua_path);
    auto& files = p->file;
    files[src] = dst;

    LOGI("添加文件 %s -> %s", src.c_str(), dst.c_str());
	return 0;
}

// add_res(src, dst) 添加文件到皮肤资源目录
// src 本地文件路径
// dst 目标文件路径
int lua_add_res(lua_State *L) {
	std::string src = luaL_checkstring(L, 1);
    std::string dst = luaL_checkstring(L, 2);
    if (src.empty() || dst.empty()) {
        return luaL_error(L, "add_res 源文件路径和目标路径不能为空");
    }
    ylis_build_state *p = lua_get_build_state(L);
    if (p == nullptr) {
        return luaL_error(L, "get_build_state failed");
    }
    src = utils::MakeAbsolutePath(src, g_lua_path);
    auto& res = p->res;
    res[src] = dst;
    LOGI("添加资源 %s -> %s", src.c_str(), dst.c_str());
	return 0;
}

// lua_add_icon(src) 设置安装包图标
// src 本地文件路径
int lua_add_icon(lua_State *L) {
	std::string src = luaL_checkstring(L, 1);
    if (src.empty()) {
        return luaL_error(L, "lua_add_icon 图标文件路径不能为空");
    }
    ylis_build_state *p = lua_get_build_state(L);
    if (p == nullptr) {
        return luaL_error(L, "get_build_state failed");
    }
    src = utils::MakeAbsolutePath(src, g_lua_path);
    p->icon_path = src;
    LOGI("添加图标 %s", src.c_str());
	return 0;
}

// lua_add_opts(opts) 添加参数
// opts 参数table
int lua_add_opts(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    ylis_build_state *p = lua_get_build_state(L);
    if (p == nullptr) {
        return luaL_error(L, "get_build_state failed");
    }

    // 遍历 Lua 表
    lua_pushnil(L);  // 首个键
    while (lua_next(L, 1) != 0) {
        // 栈上现在是：-1 为值，-2 为键
        const char *key = lua_tostring(L, -2);  // 获取键

        if (!key) {
            lua_pop(L, 1); // 弹出值，跳过无效键
            continue;
        }

        int type = lua_type(L, -1);
        switch (type) {
            case LUA_TSTRING: {
                const char *val = lua_tostring(L, -1);
                p->opt_str[key] = val ? val : "";
                break;
            }
            case LUA_TNUMBER: {
                if (lua_isinteger(L, -1)) {
                    lua_Integer val = lua_tointeger(L, -1);
                    p->opt_int[key] = static_cast<uint64_t>(val);
                } else {
                    lua_Number val = lua_tonumber(L, -1);
                    p->opt_float[key] = static_cast<double>(val);
                }
                break;
            }
            case LUA_TBOOLEAN: {
                bool val = lua_toboolean(L, -1);
                p->opt_int[key] = static_cast<uint64_t>(val ? 1 : 0);
                break;
            }
            default:
                LOGW("跳过不支持的字段类型: %s (%s)", key, lua_typename(L, type));
                break;
        }

        lua_pop(L, 1);  // 弹出值，继续下一个键值对
    }

    tabulate::Table table;
    table.add_row({"参数名", "参数值"});

    for(auto&item : p->opt_str) {
        table.add_row({item.first, item.second});
    }
    for(auto&item : p->opt_int) {
        table.add_row({item.first, std::to_string(item.second)});
    }
    for(auto&item : p->opt_float) {
        table.add_row({item.first, std::to_string(item.second)});
    }
    LOGI("BUILD参数：\n%s\n", table.str().c_str());
    return 0;
}

void register_functions(lua_State* L) {
    lua_register(L, "add_file", lua_add_file);
    lua_register(L, "add_res", lua_add_res);
    lua_register(L, "add_opts", lua_add_opts);
    lua_register(L, "add_icon", lua_add_icon);
}