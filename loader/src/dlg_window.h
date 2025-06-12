#pragma once

class DlgWindow : public ui::WindowImplBase {
  public:
	DlgWindow() = default;
	DlgWindow(const std::wstring &title, const std::wstring &ok_text = L"确定",
			  const std::wstring &no_text = L"取消", bool show_input = false, bool is_password = false);
	~DlgWindow() = default;
	std::wstring GetSkinFolder() override;
	std::wstring GetSkinFile() override;
	std::wstring GetWindowClassName() const override;
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void InitWindow() override;
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam,
							BOOL &bHandled);
	static const std::wstring kClassName;
	int ShowModal(HWND parent_hwnd);
    std::wstring GetInputText();

  private:
	int m_nRet = 0;
    std::wstring m_sTitle;
    std::wstring m_sOkText;
    std::wstring m_sNoText;
    bool m_bIsShowInput;
	bool m_bIsPassword;
};