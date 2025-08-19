#ifndef UI_CONTROL_NUMBEREDIT_H_
#define UI_CONTROL_NUMBEREDIT_H_

#pragma once

namespace ui 
{

	class UILIB_API NumberEdit : public RichEdit
	{
	public:

	public:
		NumberEdit();
		NumberEdit(const NumberEdit& r) = delete;
		NumberEdit& operator=(const NumberEdit& r) = delete;

		/**
		 * @brief 判断是否仅支持整数
		 * @return 返回 true 表示是
		 */
		bool IsIntOnly();

		/**
		 * @brief 设置是否仅支持整数
		 * @param[in] bIntOnly 为 true 则只能输入整数，false 为实数
		 * @return 无
		 */
		void SetIntOnly(bool bIntOnly = false);

		/**
		 * @brief 判断是否允许负数
		 * @return 返回 true 表示允许
		 */
		bool IsAllowNegative();

		/**
		 * @brief 设置是否允许负数
		 * @param[in] bIntOnly 为 true 则允许输入负数
		 * @return 无
		 */
		void SetAllowNegative(bool bAllowNegative = false);

		/**
 		 * @brief 设置控件的文本
 		 * @param[in] strText 要设置的文本内容
 		 * @return 无
 		 */
		virtual void SetText(const std::wstring& strText) override;
		virtual void OnChar(EventArgs& event) override;
		virtual void OnKeyDown(EventArgs& event) override;

		virtual void SetAttribute(const std::wstring& pstrName, const std::wstring& pstrValue) override;

	protected:
		bool m_bIntOnly; //仅支持整数 默认否
		bool m_bAllowNegative; //允许负数 默认否

	private:
		std::wstring _ToNumberStr(std::wstring str);
	};
	

} // namespace ui

#endif // UI_CONTROL_RICHEDIT_H_
