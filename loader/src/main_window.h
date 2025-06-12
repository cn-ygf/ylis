#pragma once

#include "dlg_window.h"
#include "lualib/luab.h"
#include "msic_thread.h"
#include "window.h"

class ShowDlgContext {
public:
    HANDLE event;
    std::wstring title;
    std::wstring ok_text;
    std::wstring no_text;
    bool show_input;
    bool is_password;
    std::wstring out;
    int ret;
};

class MainWindow : public ui::WindowImplBase, public Window {
  public:
	MainWindow(lua_State *l, const std::shared_ptr<MsicThread>& ptr, const std::shared_ptr<ylis_state>& state) {
        m_pMsicThreadPtr = ptr;
        L = l;
        state_ptr = state;
    };
	~MainWindow() = default;
	std::wstring GetSkinFolder() override;
	std::wstring GetSkinFile() override;
	std::wstring GetWindowClassName() const override;
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void InitWindow() override;
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam,
							BOOL &bHandled);
	static const std::wstring kClassName;
	void Close(UINT nRet = IDOK) override;
	void PostUpdateProgressTask(int value) override;
	void PostError() override;
	void PostInstallSuccess() override;
    void PostShowDlgTask(const std::shared_ptr<ShowDlgContext>& ptr);
    void PostEnableInstallPathTask(bool enable);

  private:
	bool init_lua(std::string &err);
	void show_path_box(bool show = true);
	bool show_dir_select_dialog(std::wstring &path,
								const std::wstring &default_path);
	void start_install(const std::string &install_path);
	void UpdateProgressTask(int value);
	void ErrorTask();
	void InstallSuccessTask();
	void ShowDlgTask(const std::shared_ptr<ShowDlgContext>& ptr);
    void EnableInstallPathTask(bool enable);
	int ShowDlg(const std::wstring &title, std::wstring &out,
				const std::wstring &ok_text = L"确定",
				const std::wstring &no_text = L"取消", bool show_input = false, bool is_password = true);
	int m_nProgress = 0;
	bool m_bIsLoading = false;
	lua_State *L;
	std::shared_ptr<MsicThread> m_pMsicThreadPtr;
	std::shared_ptr<ylis_state> state_ptr;
	std::unique_ptr<DlgWindow> m_DlgWindowPtr;
};