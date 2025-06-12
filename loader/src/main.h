#pragma once

#include "msic_thread.h"

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

  bool init_lua(std::string &err);
  lua_State *L;
	std::shared_ptr<MsicThread> m_pMsicThreadPtr;
	std::shared_ptr<ylis_state> state_ptr;
};