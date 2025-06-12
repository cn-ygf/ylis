#include "uninstall_window.h"
#include "loader.h"
#include "main_window.h"
#include "pch.h"
#include <hv/base64.h>
#include <hv/hlog.h>
#include <hv/json.hpp>
#include <utils/utils.h>

enum ThreadId {
	kThreadUI,	// UI线程
	kThreadMsic // 业务线程
};

// 显示对话框
int lua_uninstall_show_dlg(lua_State *L) {
	std::string title = luaL_checkstring(L, 1);
	std::string ok_text = luaL_optstring(L, 2, "确定");
	std::string no_text = luaL_optstring(L, 3, "取消");
	int show_input = luaL_optinteger(L, 4, 0);
	int is_password = luaL_optinteger(L, 5, 0);
	lua_getfield(L, LUA_REGISTRYINDEX, "uninstall_window");
	void *p = lua_touserdata(L, -1);
	if (p == nullptr) {
		return luaL_error(L, "uninstall_window not found");
	}
	UninstallWindow *pThis = (UninstallWindow *)p;
	std::shared_ptr<ShowDlgContext> ptr = std::make_shared<ShowDlgContext>();
	ptr->event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ptr->title = utils::UTF8ToUTF16(title);
	ptr->ok_text = utils::UTF8ToUTF16(ok_text);
	ptr->no_text = utils::UTF8ToUTF16(no_text);
	ptr->show_input = show_input == 1;
	ptr->is_password = is_password == 1;
	ptr->ret = 0;
	pThis->PostShowDlgTask(ptr);
	WaitForSingleObject(ptr->event, INFINITE);
	CloseHandle(ptr->event);
	lua_pushinteger(L, ptr->ret);
	if (show_input) {

		lua_pushstring(L, utils::UTF16ToUTF8(ptr->out).c_str());
		return 2;
	}
	return 1;
}

const std::wstring UninstallWindow::kClassName = L"UninstallClassName";

std::wstring UninstallWindow::GetSkinFolder() { return L"ylis"; }

std::wstring UninstallWindow::GetSkinFile() { return L"uninstall.xml"; }

std::wstring UninstallWindow::GetWindowClassName() const {
	return L"UninstallClassName";
}

void UninstallWindow::InitWindow() {
	SetWindowPos(GetHWND(), HWND_TOPMOST, 0, 0, 0, 0,
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SetWindowPos(GetHWND(), HWND_NOTOPMOST, 0, 0, 0, 0,
				 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	SetForegroundWindow(GetHWND());
	m_pMsicThreadPtr->SetMainWindow(this);
	std::string err;
	if (!init_lua(err)) {
		LOGE("init_lua failed. %s", err.c_str());
		return;
	}
	ui::Button *btn_success =
		dynamic_cast<ui::Button *>(FindControl(L"btn_success"));
	btn_success->AttachClick([this](ui::EventArgs *args) {
		// 点击卸载完成
		this->Close(1);
		return true;
	});

	m_bIsLoading = true;
	m_pMsicThreadPtr->PostUninstallTask();
}

LRESULT UninstallWindow::HandleMessage(UINT uMsg, WPARAM wParam,
									   LPARAM lParam) {
	return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT UninstallWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam,
								 BOOL &bHandled) {
	if (!m_bIsLoading) {
		if (m_bSuccess) {
			// 卸载脚本执行成功才删除
			std::string installdir = this->state_ptr->state["install_dir"];
			self_delete(installdir);
		}
		PostQuitMessage(0);
		return 0;
	}
	return 0;
}

void UninstallWindow::Close(UINT nRet) {
	if (!m_bIsLoading) {
		__super::Close(nRet);
	}
}

bool UninstallWindow::init_lua(std::string &err) {
	lua_register(L, "show_dlg", lua_uninstall_show_dlg);
	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, "uninstall_window");

	// 设置标题
	if (state_ptr->state.count("title")) {
		std::wstring title = utils::UTF8ToUTF16(state_ptr->state["title"]);
		::SetWindowText(GetHWND(), title.c_str());
	}
	return true;
}

void UninstallWindow::PostUpdateProgressTask(int value) {
	nbase::ThreadManager::PostTask(
		kThreadUI,
		nbase::Bind(&UninstallWindow::UpdateProgressTask, this, value));
}

void UninstallWindow::PostError() {
	nbase::ThreadManager::PostTask(
		kThreadUI, nbase::Bind(&UninstallWindow::ErrorTask, this));
}

void UninstallWindow::ErrorTask() {
	m_bIsLoading = false;
	ui::TabBox *tb_main = dynamic_cast<ui::TabBox *>(FindControl(L"tb_main"));
	tb_main->SelectItem(2);
	/*MessageBox(GetHWND(), L"安装出现错误，请联系管理员", L"ERROR",
			   MB_OK | MB_ICONERROR);*/
}

void UninstallWindow::PostInstallSuccess() {
	nbase::ThreadManager::PostTask(
		kThreadUI, nbase::Bind(&UninstallWindow::InstallSuccessTask, this));
}

void UninstallWindow::InstallSuccessTask() {
	m_bSuccess = true;
	ui::TabBox *tb_main = dynamic_cast<ui::TabBox *>(FindControl(L"tb_main"));
	if (tb_main) {
		tb_main->SelectItem(1);
	}
	m_bIsLoading = false;
}

void UninstallWindow::UpdateProgressTask(int value) {
	// m_nProgress += value;
	ui::Progress *progress =
		dynamic_cast<ui::Progress *>(FindControl(L"progress"));
	if (progress) {
		progress->SetValue(value);
	}
	ui::Label *lb_progress =
		dynamic_cast<ui::Label *>(FindControl(L"lb_progress"));
	if (lb_progress) {
		std::wstring text = nbase::StringPrintf(L"%d", value);
		text.append(L"%");
		lb_progress->SetText(text.c_str());
	}
}

// 显示对话框
int UninstallWindow::ShowDlg(const std::wstring &title, std::wstring &out,
							 const std::wstring &ok_text,
							 const std::wstring &no_text, bool show_input,
							 bool is_password) {
	m_DlgWindowPtr.reset();
	m_DlgWindowPtr = std::make_unique<DlgWindow>(title, ok_text, no_text,
												 show_input, is_password);
	m_DlgWindowPtr->Create(GetHWND(), DlgWindow::kClassName.c_str(),
						   WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, 0);
	m_DlgWindowPtr->CenterWindow();
	int ret = m_DlgWindowPtr->ShowModal(GetHWND());
	if (ret == 1 && show_input) {
		out = m_DlgWindowPtr->GetInputText();
	}
	return ret;
}

void UninstallWindow::PostShowDlgTask(
	const std::shared_ptr<ShowDlgContext> &ptr) {
	nbase::ThreadManager::PostTask(
		kThreadUI, nbase::Bind(&UninstallWindow::ShowDlgTask, this, ptr));
}

void UninstallWindow::ShowDlgTask(const std::shared_ptr<ShowDlgContext> &ptr) {
	std::wstring out;
	ptr->ret = ShowDlg(ptr->title, ptr->out, ptr->ok_text, ptr->no_text,
					   ptr->show_input, ptr->is_password);
	SetEvent(ptr->event);
}

void UninstallWindow::PostEnableInstallPathTask(bool enable) {
	nbase::ThreadManager::PostTask(
		kThreadUI,
		nbase::Bind(&UninstallWindow::EnableInstallPathTask, this, enable));
}

void UninstallWindow::EnableInstallPathTask(bool enable) {
	ui::RichEdit *rich_path =
		dynamic_cast<ui::RichEdit *>(FindControl(L"rich_path"));
	rich_path->SetEnabled(enable);
	ui::Button *btn_ll = dynamic_cast<ui::Button *>(FindControl(L"btn_ll"));
	btn_ll->SetEnabled(enable);
}
