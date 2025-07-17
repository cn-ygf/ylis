#include "main_window.h"
#include "loader.h"
#include "pch.h"
#include <hv/base64.h>
#include <hv/hlog.h>
#include <utils/utils.h>

enum ThreadId {
	kThreadUI,	// UI线程
	kThreadMsic // 业务线程
};

const std::wstring MainWindow::kClassName = L"YLISClassName";

std::wstring MainWindow::GetSkinFolder() { return L"ylis"; }

std::wstring MainWindow::GetSkinFile() { return L"main.xml"; }

std::wstring MainWindow::GetWindowClassName() const { return L"YLISClassName"; }

// 启用或禁用安装目录选择框和浏览按钮
int lua_enable_install_path(lua_State *L) {
	int enable = luaL_optinteger(L, 1, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, "main_window");
	void *p = lua_touserdata(L, -1);
	if (p == nullptr) {
		return luaL_error(L, "main_window not found");
	}
	MainWindow *pThis = (MainWindow *)p;
	pThis->PostEnableInstallPathTask(enable == 1);
	return 0;
}

// 是否开机启动
int lua_get_autorun(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "main_window");
	void *p = lua_touserdata(L, -1);
	if (p == nullptr) {
		return luaL_error(L, "main_window not found");
	}
	MainWindow *pThis = (MainWindow *)p;

	ui::CheckBox *ck_autorun =
		dynamic_cast<ui::CheckBox *>(pThis->FindControl(L"ck_autorun"));
	assert(ck_autorun != nullptr);
	lua_pushboolean(L, ck_autorun->IsSelected());
	return 1;
}

// 显示对话框
int lua_show_dlg(lua_State *L) {
	std::string title = luaL_checkstring(L, 1);
	std::string ok_text = luaL_optstring(L, 2, "确定");
	std::string no_text = luaL_optstring(L, 3, "取消");
	int show_input = luaL_optinteger(L, 4, 0);
	int is_password = luaL_optinteger(L, 5, 0);
	lua_getfield(L, LUA_REGISTRYINDEX, "main_window");
	void *p = lua_touserdata(L, -1);
	if (p == nullptr) {
		return luaL_error(L, "main_window not found");
	}
	MainWindow *pThis = (MainWindow *)p;
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

bool MainWindow::init_lua(std::string &err) {
	lua_register(L, "show_dlg", lua_show_dlg);
	lua_register(L, "enable_install_path", lua_enable_install_path);
	lua_register(L, "get_autorun", lua_get_autorun);
	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, "main_window");

	// 设置标题
	if (state_ptr->state.count("title")) {
		std::wstring title = utils::UTF8ToUTF16(state_ptr->state["title"]);
		::SetWindowText(GetHWND(), title.c_str());
	}
	return true;
}

void MainWindow::InitWindow() {
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

	ui::CheckBox *btn_show_path =
		dynamic_cast<ui::CheckBox *>(FindControl(L"btn_show_path"));
	btn_show_path->AttachSelect([this](ui::EventArgs *args) {
		this->show_path_box(true);
		return true;
	});
	btn_show_path->AttachUnSelect([this](ui::EventArgs *args) {
		this->show_path_box(false);
		return true;
	});
	ui::Button *btn_show_path_ex =
		dynamic_cast<ui::Button *>(FindControl(L"btn_show_path_ex"));
	btn_show_path_ex->AttachClick([this](ui::EventArgs *args) {
		ui::CheckBox *show_path =
			dynamic_cast<ui::CheckBox *>(FindControl(L"btn_show_path"));
		show_path->Selected(!show_path->IsSelected());
		this->show_path_box(show_path->IsSelected());
		return true;
	});

	ui::CheckBox *ck_accept =
		dynamic_cast<ui::CheckBox *>(FindControl(L"ck_accept"));
	ck_accept->AttachSelect([this](ui::EventArgs *args) {
		ui::RichEdit *rich_path =
			dynamic_cast<ui::RichEdit *>(FindControl(L"rich_path"));
		if (!utils::IsPathSyntaxValid(rich_path->GetText())) {
			return true;
		}

		ui::Button *btn_install =
			dynamic_cast<ui::Button *>(FindControl(L"btn_install"));
		btn_install->SetEnabled(true);
		return true;
	});
	ck_accept->AttachUnSelect([this](ui::EventArgs *args) {
		ui::Button *btn_install =
			dynamic_cast<ui::Button *>(FindControl(L"btn_install"));
		btn_install->SetEnabled(false);
		return true;
	});

	ui::Button *btn_link = dynamic_cast<ui::Button *>(FindControl(L"btn_link"));
	btn_link->AttachClick([this](ui::EventArgs *args) {
		if (this->state_ptr->state.count("service_agreement_url") > 0) {
			std::string url = this->state_ptr->state["service_agreement_url"];
			if (!url.empty()) {
				ShellExecuteA(
					NULL, "open",
					url.c_str(),
					NULL, NULL, SW_SHOWNORMAL);
			}
		}
		return true;
	});

	ui::Button *btn_install =
		dynamic_cast<ui::Button *>(FindControl(L"btn_install"));
	btn_install->AttachClick([this](ui::EventArgs *args) {
		ui::RichEdit *rich_path =
			dynamic_cast<ui::RichEdit *>(FindControl(L"rich_path"));
		if (!utils::IsPathSyntaxValid(rich_path->GetText())) {
			return true;
		}

		std::wstring install_path_w = rich_path->GetText();
		std::string install_path = utils::UTF16ToUTF8(install_path_w);
		std::string install_path_base64 = hv::Base64Encode(
			(const uint8_t *)install_path.data(), install_path.size());
		std::wstring install_path_base64_w =
			utils::UTF8ToUTF16(install_path_base64);
		// 进入安装流程
		this->start_install(install_path);
		return true;
	});

	ui::Button *btn_ll = dynamic_cast<ui::Button *>(FindControl(L"btn_ll"));
	btn_ll->AttachClick([this](ui::EventArgs *args) {
		std::wstring path;
		if (show_dir_select_dialog(path, L"C:\\Program Files")) {
			// path.append(L"\\MyTestSoft");
			path.append(L"\\");
			if (this->state_ptr->state.count("install_dir") > 0) {
				std::wstring install_dir =
					utils::UTF8ToUTF16(this->state_ptr->state["install_dir"]);
				path.append(install_dir);
			} else if (this->state_ptr->state.count("app_name") > 0) {
				std::wstring install_dir =
					utils::UTF8ToUTF16(this->state_ptr->state["app_name"]);
				path.append(install_dir);
			}
			ui::RichEdit *rich_path =
				dynamic_cast<ui::RichEdit *>(FindControl(L"rich_path"));
			rich_path->SetText(path);
		}
		return true;
	});

	ui::RichEdit *rich_path =
		dynamic_cast<ui::RichEdit *>(FindControl(L"rich_path"));
	rich_path->AttachTextChange([this](ui::EventArgs *args) {
		ui::RichEdit *rich_path =
			dynamic_cast<ui::RichEdit *>(FindControl(L"rich_path"));
		ui::Button *btn_install =
			dynamic_cast<ui::Button *>(FindControl(L"btn_install"));
		if (!utils::IsPathSyntaxValid(rich_path->GetText())) {
			btn_install->SetEnabled(false);
			return true;
		}
		ui::CheckBox *ck_accept =
			dynamic_cast<ui::CheckBox *>(FindControl(L"ck_accept"));
		if (ck_accept->IsSelected()) {
			btn_install->SetEnabled(true);
		}
		return true;
	});

	ui::Button *btn_run = dynamic_cast<ui::Button *>(FindControl(L"btn_run"));
	btn_run->AttachClick([this](ui::EventArgs *args) {
		// 调用 Lua 函数 run()
		m_pMsicThreadPtr->PostRunTask();
		// m_pMsicThreadPtr->WaitForRunTask();
		// this->Close();
		return true;
	});

	// 调用init()
	m_pMsicThreadPtr->PostInitTask();
	// m_pMsicThreadPtr->WaitForInitTask();
	/*if (!installdir.empty()) {
		// m_pMsicThreadPtr->Wait();
		this->start_install(installdir);
	} else {
		m_pMsicThreadPtr->PostInitTask();
	}*/
}

// 启动安装流程
void MainWindow::start_install(const std::string &install_path) {
	state_ptr->state["install_dir"] = install_path;
	show_path_box(false);

	ui::TabBox *tb_main = dynamic_cast<ui::TabBox *>(FindControl(L"tb_main"));
	tb_main->SelectItem(1);
	m_bIsLoading = true;
	m_pMsicThreadPtr->WaitForInitTask();
	m_pMsicThreadPtr->PostInstallTask();
}

bool MainWindow::show_dir_select_dialog(std::wstring &path,
										const std::wstring &default_path) {
	path.clear();
	IFileDialog *pFileDialog = nullptr;

	// 创建文件对话框实例
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
								  IID_PPV_ARGS(&pFileDialog));
	if (SUCCEEDED(hr)) {
		// 设置为选择文件夹模式
		DWORD dwOptions;
		pFileDialog->GetOptions(&dwOptions);
		pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);

		// 设置默认路径
		IShellItem *psiFolder = nullptr;
		hr = SHCreateItemFromParsingName(default_path.c_str(), NULL,
										 IID_PPV_ARGS(&psiFolder));
		if (SUCCEEDED(hr)) {
			pFileDialog->SetFolder(psiFolder); // 或用 SetDefaultFolder
			psiFolder->Release();
		}

		// 显示对话框
		hr = pFileDialog->Show(NULL);
		if (SUCCEEDED(hr)) {
			IShellItem *pItem;
			hr = pFileDialog->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				PWSTR pszFolderPath = NULL;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath);
				if (SUCCEEDED(hr)) {
					path = pszFolderPath;
					CoTaskMemFree(pszFolderPath);
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}
	return !path.empty();
}

// 是否显示安装路径box
void MainWindow::show_path_box(bool show) {
	ui::Box *box_path = dynamic_cast<ui::Box *>(FindControl(L"box_path"));
	box_path->SetVisible(show);

	int height = 0;
	int width = 550;
	if (show) {
		height = 462;
	} else {
		height = 408;
	}
	Resize(width, height);
}

LRESULT MainWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam,
							BOOL &bHandled) {
	if (!m_bIsLoading) {
		PostQuitMessage(0);
		return 0;
	}
	return 0;
}

void MainWindow::Close(UINT nRet) {
	if (!m_bIsLoading) {
		__super::Close(nRet);
	}
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_TIMER) {
		if (wParam == 0x01) {
			m_nProgress += 10;
			ui::Progress *progress =
				dynamic_cast<ui::Progress *>(FindControl(L"progress"));
			if (progress) {
				progress->SetValue(m_nProgress);
			}
			ui::Label *lb_progress =
				dynamic_cast<ui::Label *>(FindControl(L"lb_progress"));
			if (lb_progress) {
				std::wstring text = nbase::StringPrintf(L"%d", m_nProgress);
				text.append(L"%");
				lb_progress->SetText(text.c_str());
			}
			if (m_nProgress >= 100) {
				KillTimer(GetHWND(), 0x01);
				ui::TabBox *tb_main =
					dynamic_cast<ui::TabBox *>(FindControl(L"tb_main"));
				if (tb_main) {
					tb_main->SelectItem(2);
				}
				m_bIsLoading = false;
			}
		}
	}
	return __super::HandleMessage(uMsg, wParam, lParam);
}

void MainWindow::PostUpdateProgressTask(int value) {
	nbase::ThreadManager::PostTask(
		kThreadUI, nbase::Bind(&MainWindow::UpdateProgressTask, this, value));
}

void MainWindow::PostError() {
	nbase::ThreadManager::PostTask(kThreadUI,
								   nbase::Bind(&MainWindow::ErrorTask, this));
}

void MainWindow::ErrorTask() {
	m_bIsLoading = false;
	ui::TabBox *tb_main = dynamic_cast<ui::TabBox *>(FindControl(L"tb_main"));
	tb_main->SelectItem(3);
	/*MessageBox(GetHWND(), L"安装出现错误，请联系管理员", L"ERROR",
			   MB_OK | MB_ICONERROR);*/
}

void MainWindow::PostInstallSuccess() {
	nbase::ThreadManager::PostTask(
		kThreadUI, nbase::Bind(&MainWindow::InstallSuccessTask, this));
}

void MainWindow::InstallSuccessTask() {
	ui::TabBox *tb_main = dynamic_cast<ui::TabBox *>(FindControl(L"tb_main"));
	if (tb_main) {
		tb_main->SelectItem(2);
	}
	m_bIsLoading = false;
}

void MainWindow::UpdateProgressTask(int value) {
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
int MainWindow::ShowDlg(const std::wstring &title, std::wstring &out,
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

void MainWindow::PostShowDlgTask(const std::shared_ptr<ShowDlgContext> &ptr) {
	nbase::ThreadManager::PostTask(
		kThreadUI, nbase::Bind(&MainWindow::ShowDlgTask, this, ptr));
}

void MainWindow::ShowDlgTask(const std::shared_ptr<ShowDlgContext> &ptr) {
	std::wstring out;
	ptr->ret = ShowDlg(ptr->title, ptr->out, ptr->ok_text, ptr->no_text,
					   ptr->show_input, ptr->is_password);
	SetEvent(ptr->event);
}

void MainWindow::PostEnableInstallPathTask(bool enable) {
	nbase::ThreadManager::PostTask(
		kThreadUI,
		nbase::Bind(&MainWindow::EnableInstallPathTask, this, enable));
}

void MainWindow::EnableInstallPathTask(bool enable) {
	ui::RichEdit *rich_path =
		dynamic_cast<ui::RichEdit *>(FindControl(L"rich_path"));
	rich_path->SetEnabled(enable);
	ui::Button *btn_ll = dynamic_cast<ui::Button *>(FindControl(L"btn_ll"));
	btn_ll->SetEnabled(enable);
}
