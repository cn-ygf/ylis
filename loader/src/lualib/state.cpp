#include "luab.h"
#include <base/base.h>

int lua_get_string(lua_State *L) {
    std::string key = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "ylis_state");
    void* p = lua_touserdata(L, -1);
    if (p == nullptr) {
        return luaL_error(L, "ylis_state not found");
    }
    ylis_state *p_state = (ylis_state*)p;
    auto& state = p_state->state;
    if (state.count(key) < 1) {
        lua_pushstring(L, "");
        return 1;
    }
    lua_pushstring(L, state.at(key).c_str());
    return 1;
}

int lua_get_int(lua_State *L) {
    std::string key = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "ylis_state");
    void* p = lua_touserdata(L, -1);
    if (p == nullptr) {
        return luaL_error(L, "ylis_state not found");
    }
    ylis_state *p_state = (ylis_state*)p;
    auto& state = p_state->state_int;
    if (state.count(key) < 1) {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushinteger(L, state.at(key));
    return 1;
}

int lua_get_float(lua_State *L) {
    std::string key = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "ylis_state");
    void* p = lua_touserdata(L, -1);
    if (p == nullptr) {
        return luaL_error(L, "ylis_state not found");
    }
    ylis_state *p_state = (ylis_state*)p;
    auto& state = p_state->state_float;
    if (state.count(key) < 1) {
        lua_pushnumber(L, 0);
        return 1;
    }
    lua_pushnumber(L, state.at(key));
    return 1;
}