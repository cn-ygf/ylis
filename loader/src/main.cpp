#include "main.h"
#include "dlg_window.h"
#include "loader.h"
#include "lualib/luab.h"
#include "main_window.h"
#include "pch.h"
#include "uninstall_window.h"
#include <filesystem>
#include <hv/base64.h>
#include <hv/hlog.h>
#include <hv/json.hpp>
#include <utils/utils.h>

#define IDR_THEME 1001

enum ThreadId {
	kThreadUI,	// UI线程
	kThreadMsic // 业务线程
};

void my_logger(int loglevel, const char *buf, int len) {
	file_logger(loglevel, buf, len);
	// stdout_logger(loglevel, buf, len);
}

static char g_lualib_data[] = {
#include "lualib.lua.h"
};

// 静默安装兼容
int lua_show_dlg_none(lua_State *L);

// 静默安装兼容
int lua_enable_install_path_none(lua_State *L);
int lua_get_autorun_none(lua_State *L);

nbase::CmdLineArgs *args;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
					  _In_ int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	args = new nbase::CmdLineArgs(lpCmdLine);

	std::error_code ec;
	char szTempPath[MAX_PATH] = {0};
	GetTempPathA(MAX_PATH, szTempPath);
	std::string temp_path = szTempPath;
	temp_path.append("{B6960550-71F9-4F7C-8426-B6B271E92581}");
	if (!std::filesystem::exists(temp_path)) {
		std::filesystem::create_directories(temp_path, ec);
	}
	std::string log_file = temp_path + "\\ylis.log";

	hlog_set_file(log_file.c_str());
	hlog_set_level(LOG_LEVEL_INFO);
	// hlog_set_level(LOG_LEVEL_DEBUG);
	hlog_set_handler(my_logger);

	if (!nbase::win32::IsUserAdmin()) {
		// 请求UAC运行并带上参数运行
		std::wstring bin_path = nbase::win32::GetModulePathName(NULL);
		SHELLEXECUTEINFO sei = {sizeof(sei)};
		sei.lpVerb = _T("runas");
		sei.lpFile = bin_path.c_str();
		sei.nShow = SW_NORMAL;
		sei.lpParameters = lpCmdLine;
		if (!ShellExecuteEx(&sei)) {
			DWORD dwError = GetLastError();
			if (dwError == ERROR_CANCELLED) {
				LOGE("User cancel uac: %d", dwError);
			} else {
				LOGE("ShellExecuteEx failed,dwCode: %d", dwError);
			}
		}
		ExitProcess(0);
		return 0;
	}

	// 创建主线程
	MainThread thread;

	// 执行主线程循环
	thread.RunOnCurrentThreadWithLoop(nbase::MessageLoop::kUIMessageLoop);

	delete args;
	return 0;
}

void MainThread::Init() {
	nbase::ThreadManager::RegisterThread(kThreadUI);

	// 获取资源路径，初始化全局参数
	// std::wstring theme_dir = nbase::win32::GetCurrentModuleDirectory();
#ifdef _DEBUG
	// std::wstring theme_dir = nbase::win32::GetCurrentModuleDirectory();
	// Debug 模式下使用本地文件夹作为资源
	// 默认皮肤使用 resources\\themes\\default
	// 默认语言使用 resources\\lang\\zh_CN
	// 如需修改请指定 Startup 最后两个参数
	ui::GlobalManager::OpenResZip(MAKEINTRESOURCE(IDR_THEME), L"THEME", "");
	ui::GlobalManager::Startup(L"resources\\", ui::CreateControlCallback(),
							   false);
	/*std::wstring theme_dir = L"E:\\Code\\ylis\\";
	ui::GlobalManager::Startup(theme_dir + L"resources\\",
							   ui::CreateControlCallback(), false);*/

#else
	// Release 模式下使用资源中的压缩包作为资源
	// 资源被导入到资源列表分类为 THEME，资源名称为 IDR_THEME
	// 如果资源使用的是本地的 zip 文件而非资源中的 zip 压缩包
	// 可以使用 OpenResZip 另一个重载函数打开本地的资源压缩包
	ui::GlobalManager::OpenResZip(MAKEINTRESOURCE(IDR_THEME), L"THEME", "");
	// ui::GlobalManager::OpenResZip(L"resources.zip", "");
	ui::GlobalManager::Startup(L"resources\\", ui::CreateControlCallback(),
							   false);
#endif

	std::string err;
	if (!init_lua(err)) {
		LOGE("init_lua failed, err: %s", err.c_str());
		this->Stop();
		return;
	}

	// 获取命令行参数

	std::string installdir;
	if (args->size() > 0) {
		std::wstring cmd = args->at(0);
		if (cmd == L"--quick" || cmd == L"--updated") {
			// 快速静默安装
			std::string installdir = "C:\\Program Files\\";
			if (this->state_ptr->state.count("install_dir") > 0) {
				installdir.append(this->state_ptr->state["install_dir"]);
			} else if (this->state_ptr->state.count("app_name") > 0) {
				installdir.append(this->state_ptr->state["app_name"]);
			} else {
				LOGE("quick install failed, not found installdir.");
				ExitProcess(0);
			}
			this->state_ptr->state["install_dir"] = installdir;

			lua_register(this->L, "show_dlg", lua_show_dlg_none);
			lua_register(this->L, "enable_install_path",
						 lua_enable_install_path_none);
			lua_register(this->L, "get_autorun", lua_get_autorun_none);

			this->m_pMsicThreadPtr->PostInitTask();
			this->m_pMsicThreadPtr->WaitForInitTask();
			if (this->m_pMsicThreadPtr->GetInitState()) {
				this->m_pMsicThreadPtr->PostInstallTask();
				this->m_pMsicThreadPtr->WaitForInstallTask();
			}
			this->m_pMsicThreadPtr->Stop();
			ExitProcess(0);
			return;
		}
	}

	// 判断是不是卸载
	std::wstring pe_name = nbase::win32::GetModuleName(NULL);
	if (pe_name == L"uninstall.exe") {
		// 卸载流程
		DlgWindow dlg(L"真的要卸载吗？");
		dlg.Create(NULL, DlgWindow::kClassName.c_str(),
				   WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, 0);
		dlg.CenterWindow();
		int ret = dlg.ShowModal(NULL);
		if (ret == 1) {
			// TODO 给MsicPost卸载流程
			UninstallWindow *window =
				new UninstallWindow(L, m_pMsicThreadPtr, state_ptr);
			window->Create(NULL, UninstallWindow::kClassName.c_str(),
						   WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, 0);
			window->CenterWindow();
			window->ShowWindow();
		} else {
			ExitProcess(0);
		}
		return;
	}

	// 创建一个默认带有阴影的居中窗口
	MainWindow *window = new MainWindow(L, m_pMsicThreadPtr, state_ptr);
	window->Create(NULL, MainWindow::kClassName.c_str(),
				   WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, 0);
	window->CenterWindow();
	window->ShowWindow();
}

void MainThread::Cleanup() {
	ui::GlobalManager::Shutdown();
	SetThreadWasQuitProperly(true);
	nbase::ThreadManager::UnregisterThread();
}

bool MainThread::init_lua(std::string &err) {
	L = luaL_newstate();
	luaL_openlibs(L);
	// 注册cpp函数
	register_functions(L);
	// 注册lua函数
	if (luaL_dostring(L, g_lualib_data) != LUA_OK) {
		err = nbase::StringPrintf("registry lualib failed: %s",
								  lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);
		return false;
	}

	// 读取资源脚本和opt
	std::string opt_base64;
	if (!get_resource(IDR_THEME_OPT, "THEME", opt_base64, err)) {
		err = "get_resource IDR_THEME_OPT failed";
		lua_pop(L, 1);
		lua_close(L);
		return false;
	}

	// 设置状态
	state_ptr = std::make_shared<ylis_state>();
	lua_pushlightuserdata(L, state_ptr.get());
	lua_setfield(L, LUA_REGISTRYINDEX, "ylis_state");
	// 解析opt
	std::string opt_json_str = hv::Base64Decode(opt_base64.c_str());
	nlohmann::json j = nlohmann::json::parse(opt_json_str);
	for (auto &[key, value] : j.items()) {
		if (value.is_string()) {
			state_ptr->state[key] = value.get<std::string>();
			// LOGI("%s:%s",key.c_str(), state_ptr->state[key].c_str());
		} else if (value.is_number_integer()) {
			state_ptr->state_int[key] = value.get<int64_t>();
			// LOGI("%s:%d",key.c_str(), state_ptr->state_int[key]);
		} else if (value.is_number_float()) {
			state_ptr->state_float[key] = value.get<double>();
			// LOGI("%s:%.f",key.c_str(), state_ptr->state_float[key]);
		}
	}

	// 解析lua script
	std::string lua_base64;
	if (!get_resource(IDR_THEME_LUA, "THEME", lua_base64, err)) {
		err = "get_resource IDR_THEME_LUA failed";
		lua_pop(L, 1);
		lua_close(L);
		return false;
	}
	std::string lua_str = hv::Base64Decode(lua_base64.c_str());
	if (luaL_dostring(L, lua_str.c_str()) != LUA_OK) {
		err = nbase::StringPrintf("luaL_dostring failed: %s",
								  lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);
		return false;
	}
	// 卸载例程解析安装目录
	std::string install_path_base64;
	if (get_resource(IDR_THEME_SET, "THEME", install_path_base64, err)) {
		std::string install_dir = hv::Base64Decode(install_path_base64.c_str());
		state_ptr->state["install_dir"] = install_dir;
	}
	err.clear();

	// 启动业务线程
	m_pMsicThreadPtr = std::make_shared<MsicThread>(L, state_ptr, nullptr);
	m_pMsicThreadPtr->Start();
	return true;
}

// 启用或禁用安装目录选择框和浏览按钮
/*int lua_enable_install_path(lua_State *L) {
	int enable = luaL_optinteger(L, 1, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, "main_window");
	void *p = lua_touserdata(L, -1);
	if (p == nullptr) {
		return luaL_error(L, "main_window not found");
	}
	MainWindow *pThis = (MainWindow *)p;
	pThis->PostEnableInstallPathTask(enable == 1);
	return 0;
}*/

// 静默安装兼容
int lua_show_dlg_none(lua_State *L) {
	lua_pushinteger(L, 1);
	return 1;
}

// 静默安装兼容
int lua_enable_install_path_none(lua_State *L) { return 0; }

// 静默安装兼容
int lua_get_autorun_none(lua_State *L) {
	lua_pushboolean(L, true);
	return 1;
}