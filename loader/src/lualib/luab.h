#pragma once

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <string>
#include <map>
#include <cstdint>

struct ylis_state {
    std::map<std::string, std::string> state;
    std::map<std::string, int64_t> state_int;
    std::map<std::string, double> state_float;
};

int lua_sleep(lua_State *L);
int lua_logd(lua_State* L);
int lua_logi(lua_State* L);
int lua_logw(lua_State* L);
int lua_loge(lua_State* L);
int lua_logf(lua_State* L);
int lua_write_file_string(lua_State *L);
int lua_cp(lua_State* L);
int lua_mv(lua_State* L);
int lua_rm(lua_State* L);
int lua_mkdir(lua_State* L);
int lua_exist(lua_State* L);
int lua_get_local_app_data_dir(lua_State* L);
int lua_get_roaming_app_data_dir(lua_State *L);
int lua_exec(lua_State* L);
int lua_execn(lua_State* L);
int lua_create_process(lua_State *L);
int lua_create_process_wait(lua_State *L);
int lua_get_os_version(lua_State* L);
int lua_is_admin(lua_State* L);
int lua_get_home_dir(lua_State* L);
int lua_get_reg_string(lua_State *L);
int lua_set_reg_string(lua_State *L);
int lua_set_reg_string_ex(lua_State *L);
int lua_set_reg_multistring(lua_State *L);
int lua_get_reg_dword(lua_State *L);
int lua_set_reg_dword(lua_State *L);
int lua_del_reg(lua_State* L);
int lua_del_reg_value(lua_State* L);
int lua_get_float(lua_State *L);
int lua_get_int(lua_State *L);
int lua_get_string(lua_State *L);
int lua_md5(lua_State *L);
int lua_md5_file(lua_State *L);
int lua_sha256(lua_State *L);
int lua_base64_encode(lua_State *L);
int lua_base64_decode(lua_State *L);
int lua_create_shortcut(lua_State *L);
int lua_remove_shortcut(lua_State *L);
int lua_create_startmenu_shortcut(lua_State *L);
int lua_remove_startmenu_shortcut(lua_State *L);
int lua_get_module_file_name(lua_State *L);
int lua_get_command_line(lua_State *L);
int lua_get_args(lua_State* L);
int lua_create_service(lua_State* L);
int lua_start_service(lua_State* L);
int lua_change_service_start_mode(lua_State* L);
int lua_change_service_failure_restart(lua_State *L);
int lua_stop_service(lua_State* L);
int lua_delete_service(lua_State *L);
int lua_query_service_status(lua_State* L);
int lua_service_status_to_string(lua_State *L);
int lua_kill_process(lua_State* L);
int lua_get_process(lua_State *L);
int lua_get_physical_ip_address_list(lua_State *L);
int lua_get_physical_mac_address_list(lua_State *L);
int lua_check_mutex(lua_State *L);
int lua_exit_process(lua_State *L);
int lua_isdir(lua_State *L);
int lua_broadcast_environment_change(lua_State *L);

void register_functions(lua_State* L);
