#include <SDKDDKVer.h>
#include <windows.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <shobjidl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <fstream>

#include <base/base.h>
#include <duilib/UIlib.h>

#define kThreadUI 1

class MainWindow : public ui::WindowImplBase {
public:
	MainWindow() = default;
	~MainWindow() = default;
    std::wstring GetSkinFolder() override;
    std::wstring GetSkinFile() override;
    std::wstring GetWindowClassName() const override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    void InitWindow() override;
    virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    static const std::wstring kClassName;
};

class MainThread : public nbase::FrameworkThread {
public:
  MainThread() : nbase::FrameworkThread("MainThread") {}
  virtual ~MainThread() {}

private:
  /**
   * 虚函数，初始化主线程
   * @return void	无返回值
   */
  virtual void Init() override;

  /**
   * 虚函数，主线程退出时，做一些清理工作
   * @return void	无返回值
   */
  virtual void Cleanup() override;
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
					  _In_ int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	

	// 创建主线程
	MainThread thread;

	// 执行主线程循环
	thread.RunOnCurrentThreadWithLoop(nbase::MessageLoop::kUIMessageLoop);

	return 0;
}


void MainThread::Init() {
	nbase::ThreadManager::RegisterThread(kThreadUI);

	// std::wstring theme_dir = nbase::win32::GetCurrentModuleDirectory();
	// Debug 模式下使用本地文件夹作为资源
	// 默认皮肤使用 resources\\themes\\default
	// 默认语言使用 resources\\lang\\zh_CN
	// 如需修改请指定 Startup 最后两个参数
	std::wstring theme_dir = L"E:\\Code\\ylis\\";
	ui::GlobalManager::Startup(theme_dir + L"resources\\",
							   ui::CreateControlCallback(), false);


	// 创建一个默认带有阴影的居中窗口
	MainWindow *window = new MainWindow();
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

const std::wstring MainWindow::kClassName = L"TestClassName";

std::wstring MainWindow::GetSkinFolder() { return L"ylis"; }

std::wstring MainWindow::GetSkinFile() { return L"uninstall.xml"; }

std::wstring MainWindow::GetWindowClassName() const { return L"TestClassName"; }

void MainWindow::InitWindow() {}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT MainWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam,
							BOOL &bHandled) {
	PostQuitMessage(0);
	return 0;
}