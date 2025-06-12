#pragma once
#include "lualib/luab.h"
class Window;
class MsicThread : public nbase::FrameworkThread {
  public:
	MsicThread(lua_State *l, const std::shared_ptr<ylis_state> &p,
			   Window *window)
		: nbase::FrameworkThread("MsicThread") {
		L = l;
		ptr = p;
		m_pMainWindow = window;
		m_hInitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		m_hInstallEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		m_hRunEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	}
	virtual ~MsicThread() {}
	void PostInstallTask();
	void PostInitTask();
	void PostUninstallTask();
	void PostRunTask();
	void SetMainWindow(Window *window) { m_pMainWindow = window; }
	void WaitForInitTask();
	void WaitForInstallTask();
	void WaitForRunTask();
	bool GetInitState() { return m_bInitState; }
	bool GetInstallState() { return m_bInstallState; }
	bool GetRunState() { return m_bRunState; }

  private:
	void Init() override;
	void Cleanup() override;
	void InitTask();
	void InstallTask();
	void UninstallTask();
	void RunTask();

  private:
	lua_State *L;
	std::shared_ptr<ylis_state> ptr;
	Window *m_pMainWindow;
	HANDLE m_hInitEvent;
	HANDLE m_hInstallEvent;
	HANDLE m_hRunEvent;
	bool m_bInitState = false;
	bool m_bInstallState = false;
	bool m_bRunState = false;
};