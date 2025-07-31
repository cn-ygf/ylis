#include <windows.h>
#include "luab.h"
#include <base/base.h>
#include <utils.h>
#include <hv/hlog.h>

int lua_execn(lua_State* L) {
    const char* cmd = luaL_checkstring(L, 1);
    std::string stdOut,stdErr;
    BOOL bRet = utils::exec_command(cmd, stdOut, stdErr);
    if (!bRet) {
        LOGE("exec %s failed", cmd);
        LOGE("%s", stdOut.c_str());
        LOGE("%s", stdErr.c_str());
        return 0;
    }
    return 0;
}


int lua_exec(lua_State *L) {
    const char* cmd = luaL_checkstring(L, 1);
    std::string stdOut,stdErr;
    BOOL bRet = utils::exec_command(cmd, stdOut, stdErr);
    if (!bRet) {
        LOGE("exec %s failed", cmd);
        LOGE("%s", stdOut.c_str());
        LOGE("%s", stdErr.c_str());
        return luaL_error(L, "exec_command error");
    }
    lua_pushstring(L, stdOut.c_str());
    return 1;
}

int lua_create_process(lua_State *L) {
    const char* cmd = luaL_checkstring(L, 1);
    bool b = utils::create_process(cmd);
    lua_pushboolean(L, b);
    return 1;
}

int lua_create_process_wait(lua_State *L) {
    const char* cmd = luaL_checkstring(L, 1);
    bool b = utils::create_process_wait(cmd);
    lua_pushboolean(L, b);
    return 1;
}