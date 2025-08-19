#include "StdAfx.h"
#include "base/thread/thread_manager.h"

// These constants are for backward compatibility. They are the 
// sizes used for initialization and reset in RichEdit 1.0

namespace ui {

	NumberEdit::NumberEdit() : 
		RichEdit(), 
		m_bIntOnly(false),
		m_bAllowNegative(false)
	{
		m_bWantTab = false;
		m_lTwhStyle &= ~ES_MULTILINE;
	}

	bool NumberEdit::IsIntOnly()
	{
		return m_bIntOnly;
	}

	void NumberEdit::SetIntOnly(bool bIntOnly)
	{
		m_bIntOnly = bIntOnly;
	}

	bool NumberEdit::IsAllowNegative()
	{
		return m_bAllowNegative;
	}

	void NumberEdit::SetAllowNegative(bool bAllowNegative)
	{
		m_bAllowNegative = bAllowNegative;
	}

	void NumberEdit::SetText(const std::wstring & strText)
	{
		std::wstring strNum = _ToNumberStr(strText);
		m_sText = strNum;
		if (!m_bInited)
			return;

		SetSel(0, -1);
		ReplaceSel(strNum, FALSE);

		m_linkInfo.clear();
	}

	void NumberEdit::OnChar(EventArgs& event)
	{
		//TAB
		if (::GetKeyState(VK_TAB) < 0) 
		{
			if (m_pWindow != NULL)
				m_pWindow->SendNotify((Control*)this, kEventTab);

			return;
		}

		if (event.wParam < '0' || event.wParam > '9') //输入的是非数字
		{
			if (event.wParam == 46) //输入的是小数点
			{
				if (m_bIntOnly) //如果仅支持整数
				{
					return;
				}
				else
				{
					if (GetTextLength() == 0) 
					{
						return;
					}
					else if (GetText().find('.') != -1) 
					{
						return;
					}
					else if ((GetTextLength() == 1) && (GetText().find('-') == 0)) 
					{
						return;
					}
				}
			}
			else if (event.wParam == 45) //输入的是减号
			{
				if (!m_bAllowNegative) //如果不允许负数
				{
					return;
				}
				else
				{
					if (GetTextLength() > 0) 
					{
						return;
					}
					else if (GetText().find('-') != -1) 
					{
						return;
					}
				}
			}
			else
			{
				return;
			}
		}

		TxSendMessage(WM_CHAR, event.wParam, event.lParam, NULL);
	}

	void NumberEdit::OnKeyDown(EventArgs & event)
	{
		if (event.wParam == VK_RETURN && ::GetAsyncKeyState(VK_SHIFT) >= 0)
		{
			if (m_bNeedReturnMsg && ((m_bReturnMsgWantCtrl && ::GetAsyncKeyState(VK_CONTROL) < 0) ||
				(!m_bReturnMsgWantCtrl && ::GetAsyncKeyState(VK_CONTROL) >= 0)))
			{
				if (m_pWindow != NULL)
					m_pWindow->SendNotify((Control*)this, kEventReturn);
				return;
			}
		}
		else if (event.wParam == 'V' && ::GetKeyState(VK_CONTROL) < 0) {
			std::wstring strClipText;
			GetClipboardText(strClipText);
			if (!strClipText.empty()) 
			{
				std::wstring strNum = _ToNumberStr(strClipText);
				SetClipBoardText(strNum); //修改剪切板内容为纯数字
				nbase::ThreadManager::PostTask([strClipText]() { SetClipBoardText(strClipText); }); //粘贴完后又把剪切板内容改回来
			}
		}

		TxSendMessage(WM_KEYDOWN, event.wParam, event.lParam, NULL);
	}

	void NumberEdit::SetAttribute(const std::wstring& strName, const std::wstring& strValue)
	{
		if (strName == _T("number")) {}
		else if (strName == _T("wanttab")) {}
		else if (strName == _T("multiline")) {}
		else if (strName == _T("intonly")) SetIntOnly(strValue == _T("true"));
		else if (strName == _T("allownegative")) SetAllowNegative(strValue == _T("true"));
		else RichEdit::SetAttribute(strName, strValue);
	}

	std::wstring NumberEdit::_ToNumberStr(std::wstring str)
	{
		std::wstring strNum;
		for (auto it = str.begin(); it != str.end(); it++)
		{
			if ((*it) <= L'9' && (*it) >= L'0') {
				strNum.push_back((*it));
			}
			else if ((*it) == L'.')
			{
				if (!m_bIntOnly) //如果支持实数
				{
					if ((strNum.length() == 0) || ((strNum.length() == 1) && (strNum.find('-') == 0))) //如果是输入内容的首位，前面加0
					{
						strNum += L"0.";
						//strNum.push_back((L'0'));
						//strNum.push_back((*it));
					}
					else if (strNum.find('.') == -1) //如果尚未有小数点，允许此次输入
					{
						strNum.push_back((*it));
					}
				}
			}
			else if ((*it) == L'-')
			{
				if (m_bAllowNegative) //如果允许负数
				{
					if (strNum.length() == 0) //如果是输入内容的首位，允许
					{
						strNum.push_back((*it));
					}
				}
			}
		}

		return strNum;
	}

} // namespace ui
