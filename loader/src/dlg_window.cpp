#include "dlg_window.h"
#include "pch.h"

const std::wstring DlgWindow::kClassName = L"DlgClassName";

std::wstring DlgWindow::GetSkinFolder() { return L"ylis"; }

std::wstring DlgWindow::GetSkinFile() { return L"dlg.xml"; }

std::wstring DlgWindow::GetWindowClassName() const { return L"DlgClassName"; }

DlgWindow::DlgWindow(const std::wstring &title, const std::wstring &ok_text,
					 const std::wstring &no_text, bool show_input, bool is_password) {
	m_sTitle = title;
	m_sOkText = ok_text;
	m_sNoText = no_text;
	m_bIsShowInput = show_input;
	m_bIsPassword = is_password;
}

void DlgWindow::InitWindow() {
	ui::Label *lb_title = dynamic_cast<ui::Label *>(FindControl(L"dlg_title"));
	lb_title->SetText(m_sTitle);

	ui::Button *btn_ok = dynamic_cast<ui::Button *>(FindControl(L"btn_ok"));
	btn_ok->SetText(m_sOkText);
	btn_ok->AttachClick([this](ui::EventArgs *args) {
		this->m_nRet = 1;
		this->Close(1);
		return true;
	});
	ui::Button *btn_no = dynamic_cast<ui::Button *>(FindControl(L"btn_no"));
	btn_no->SetText(m_sNoText);
	btn_no->AttachClick([this](ui::EventArgs *args) {
		this->m_nRet = 0;
		this->Close(0);
		return true;
	});
	if (m_bIsShowInput) {
		ui::HBox *hb_form = dynamic_cast<ui::HBox *>(FindControl(L"hb_form"));
		hb_form->SetVisible(true);
	}
	if (m_bIsPassword) {
		ui::RichEdit *edit_dlg_input =
		dynamic_cast<ui::RichEdit *>(FindControl(L"edit_dlg_input"));
		edit_dlg_input->SetPassword(true);
	}
}

LRESULT DlgWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT DlgWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam,
						   BOOL &bHandled) {
	return __super::OnClose(uMsg, wParam, lParam, bHandled);
}

// 模态对话框显示封装
int DlgWindow::ShowModal(HWND parent_hwnd) {
	if (parent_hwnd) {
		__super::ShowModalFake(parent_hwnd);
	} else {
		__super::ShowWindow();
	}
	
	MSG msg = {};
	while (IsWindow(GetHWND())) {
		if (::GetMessage(&msg, nullptr, 0, 0)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		} else {
			break;
		}
	}

	return m_nRet;
}

// 获取input文本
std::wstring DlgWindow::GetInputText() {
	ui::RichEdit *edit_dlg_input =
		dynamic_cast<ui::RichEdit *>(FindControl(L"edit_dlg_input"));
	return edit_dlg_input->GetText();
}