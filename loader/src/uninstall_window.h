#pragma once
#include "lualib/luab.h"
#include "msic_thread.h"
#include "main_window.h"
#include "dlg_window.h"
#include "window.h"

class UninstallWindow : public ui::WindowImplBase, public Window {
  public:
	UninstallWindow() = default;
	UninstallWindow(lua_State *l, const std::shared_ptr<MsicThread> &ptr,
					const std::shared_ptr<ylis_state> &state) {
		m_pMsicThreadPtr = ptr;
		L = l;
		state_ptr = state;
	};
	~UninstallWindow() = default;
	std::wstring GetSkinFolder() override;
	std::wstring GetSkinFile() override;
	std::wstring GetWindowClassName() const override;
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void InitWindow() override;
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam,
							BOOL &bHandled);
	static const std::wstring kClassName;
	void Close(UINT nRet) override;
	void PostUpdateProgressTask(int value) override;
	void PostError() override;
	void PostInstallSuccess() override;
    void PostShowDlgTask(const std::shared_ptr<ShowDlgContext>& ptr);
    void PostEnableInstallPathTask(bool enable);

  private:
	bool init_lua(std::string &err);
	void UpdateProgressTask(int value);
	void ErrorTask();
	void InstallSuccessTask();
	void ShowDlgTask(const std::shared_ptr<ShowDlgContext> &ptr);
	void EnableInstallPathTask(bool enable);
	int ShowDlg(const std::wstring &title, std::wstring &out,
				const std::wstring &ok_text = L"确定",
				const std::wstring &no_text = L"取消", bool show_input = false,
				bool is_password = true);

  private:
	int m_nProgress = 0;
	bool m_bIsLoading = false;
	lua_State *L;
	std::shared_ptr<MsicThread> m_pMsicThreadPtr;
	std::shared_ptr<ylis_state> state_ptr;
	std::unique_ptr<DlgWindow> m_DlgWindowPtr;
	bool m_bSuccess = false;
};