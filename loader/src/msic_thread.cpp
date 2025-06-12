#include "msic_thread.h"
#include "loader.h"
#include "pch.h"
#include "window.h"
#include <filesystem>
#include <fstream>
#include <hv/hlog.h>
#include <utils/utils.h>

enum ThreadId {
	kThreadUI,	// UI线程
	kThreadMsic // 业务线程
};

void MsicThread::Init() {
	LOGI("%s", " MsicThread::Init()");
	nbase::ThreadManager::RegisterThread(kThreadMsic);
}

void MsicThread::Cleanup() {
	LOGI("%s", " MsicThread::Cleanup()");
	nbase::ThreadManager::UnregisterThread();
	CloseHandle(m_hInitEvent);
	CloseHandle(m_hInstallEvent);
}
void MsicThread::PostRunTask() {
	LOGI("%s", " MsicThread::PostTask()");
	nbase::ThreadManager::PostTask(kThreadMsic,
								   nbase::Bind(&MsicThread::RunTask, this));
}
void MsicThread::RunTask() {
	// 调用 Lua 函数 run()
	lua_getglobal(L, "run");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			LOGE("call run() failed: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			if (m_pMainWindow)
				m_pMainWindow->PostError();
			SetEvent(m_hRunEvent);
			return;
		}
	} else {
		LOGE("run() not found");
	}
	m_bRunState = true;
	SetEvent(m_hRunEvent);
}

void MsicThread::PostInitTask() {
	LOGI("%s", " MsicThread::PostInitTask()");
	nbase::ThreadManager::PostTask(kThreadMsic,
								   nbase::Bind(&MsicThread::InitTask, this));
}

void MsicThread::InitTask() {
	// 调用 Lua 函数 init()
	lua_getglobal(L, "init");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			LOGE("call init() failed: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			if (m_pMainWindow)
				m_pMainWindow->PostError();
			SetEvent(m_hInitEvent);
			return;
		}
	} else {
		LOGE("init() not found");
		lua_pop(L, 1);
		if (m_pMainWindow)
			m_pMainWindow->PostError();
		SetEvent(m_hInitEvent);
		return;
	}
	m_bInitState = true;
	SetEvent(m_hInitEvent);
}

void MsicThread::PostUninstallTask() {
	LOGI("%s", " MsicThread::PostUninstallTask()");
	nbase::ThreadManager::PostTask(
		kThreadMsic, nbase::Bind(&MsicThread::UninstallTask, this));
}

// 卸载过程
void MsicThread::UninstallTask() {
	if (m_pMainWindow)
		m_pMainWindow->PostUpdateProgressTask(5);
	// 调用 Lua 函数 uninstall()
	lua_getglobal(L, "uninstall");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			LOGE("call uninstall() failed: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			if (m_pMainWindow)
				m_pMainWindow->PostError();
			return;
		}
	} else {
		lua_pop(L, 1);
		LOGE("uninstall() not found");
		if (m_pMainWindow)
			m_pMainWindow->PostError();
		return;
	}
	if (m_pMainWindow)
		this->m_pMainWindow->PostUpdateProgressTask(60);
	// 删除安装目录
	std::string installdir = ptr->state["install_dir"];
	std::error_code ec;
	if (std::filesystem::exists(installdir, ec)) {
		std::filesystem::remove_all(installdir, ec);
		if (ec) {
			LOGE("%s", utils::LocalToUTF8(ec.message()).c_str());
		}
	}
	if (m_pMainWindow) {
		this->m_pMainWindow->PostUpdateProgressTask(100);
		this->m_pMainWindow->PostInstallSuccess();
	}

	// 调用自杀式删除安装目录
	// self_delete(installdir);
}

void MsicThread::PostInstallTask() {
	LOGI("%s", " MsicThread::PostInstallTask()");
	nbase::ThreadManager::PostTask(kThreadMsic,
								   nbase::Bind(&MsicThread::InstallTask, this));
}

// 安装过程
void MsicThread::InstallTask() {

	// 释放文件之前调用 before_release 用来结束进程之类的，防止被占用
	lua_getglobal(L, "before_release");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			LOGE("call before_release() failed: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			if (m_pMainWindow)
				m_pMainWindow->PostError();
			SetEvent(m_hInstallEvent);
			return;
		}
	} else {
		lua_pop(L, 1);
		LOGE("before_release() not found");
	}

	if (m_pMainWindow)
		m_pMainWindow->PostUpdateProgressTask(5);

	// 释放文件
	std::string installdir = ptr->state["install_dir"];
	try {
		if (std::filesystem::exists(installdir)) {
			std::filesystem::remove_all(installdir);
		}
	} catch (const std::exception &e) {
		LOGE("%s", e.what());
	}
	try {
		std::filesystem::create_directories(installdir);
	} catch (const std::exception &e) {
		LOGE("%s", e.what());
		if (m_pMainWindow)
			m_pMainWindow->PostError();
		SetEvent(m_hInstallEvent);
		return;
	}

	// 从pe文件读取zip的offset
	char szPeFileName[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, szPeFileName, MAX_PATH);
	std::string pe_file = szPeFileName;
	uint64_t offset, size;
	std::string err;
	ylis_header header = {0};
	if (!locate_install_data(pe_file, header, err)) {
		LOGE("%s", err.c_str());
		if (m_pMainWindow)
			m_pMainWindow->PostError();
		SetEvent(m_hInstallEvent);
		return;
	}
	if (!check_install_data(pe_file, &header, err)) {
		LOGE("%s", err.c_str());
		if (m_pMainWindow)
			m_pMainWindow->PostError();
		SetEvent(m_hInstallEvent);
		return;
	}
	LOGI("installdir: %s", installdir.c_str());
	// LOGI("PEFILE:%s offset: %d size: %d", pe_file.c_str(), offset, size);
	if (m_pMainWindow)
		m_pMainWindow->PostUpdateProgressTask(10);
	// 从解压zip到安装目录
	if (!release_install_data_from_pe(
			pe_file, header.offset, header.size, installdir, err,
			[this](int value) {
				if (m_pMainWindow)
					this->m_pMainWindow->PostUpdateProgressTask(value);
			})) {
		LOGE("%s", err.c_str());
		if (m_pMainWindow)
			m_pMainWindow->PostError();
		SetEvent(m_hInstallEvent);
		return;
	}
	// 写入pe文件头部到安装目录文件名为uninstall.exe
	if (!writer_uninstall_file(pe_file, &header, installdir, err)) {
		LOGE("%s", err.c_str());
	}

	// 运行install()
	// 调用 Lua 函数 install()
	lua_getglobal(L, "install");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			LOGE("call install() failed: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			if (m_pMainWindow)
				m_pMainWindow->PostError();
			SetEvent(m_hInstallEvent);
			return;
		}
	} else {
		lua_pop(L, 1);
		LOGE("install() not found");
		if (m_pMainWindow)
			m_pMainWindow->PostError();
		SetEvent(m_hInstallEvent);
		return;
	}
	if (m_pMainWindow)
		this->m_pMainWindow->PostUpdateProgressTask(100);
	// TODO进入第三个完成页面
	if (m_pMainWindow)
		this->m_pMainWindow->PostInstallSuccess();
	m_bInstallState = true;
	SetEvent(m_hInstallEvent);
}

void MsicThread::WaitForInitTask() {
	WaitForSingleObject(m_hInitEvent, INFINITE);
	ResetEvent(m_hInitEvent);
}

void MsicThread::WaitForInstallTask() {
	WaitForSingleObject(m_hInstallEvent, INFINITE);
	ResetEvent(m_hInstallEvent);
}

void MsicThread::WaitForRunTask() {
	WaitForSingleObject(m_hRunEvent, INFINITE);
	ResetEvent(m_hRunEvent);
}