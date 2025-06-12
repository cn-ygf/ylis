function build()
    add_file("DebugView\\DbgView.chm", "DbgView.chm")
    add_file("DebugView\\Dbgview.exe", "DbgView.exe")
    add_file("DebugView\\dbgview64.exe", "dbgview64.exe")
    add_file("DebugView\\dbgview64a.exe", "dbgview64a.exe")
    add_file("DebugView\\Eula.txt", "子目录/Eula.txt")
    add_file("DebugView\\中文文件.txt", "中文文件.txt")

    add_res("logo.png", "logo.png")
    add_icon("logo.ico")


    mytable = {}
    mytable["out"] = "debugview-v0.0.1.exe"
    mytable["guid"] = "{91E09AE7-3B22-4318-9383-788F1DD69E11}"      -- 软件GUID，用于在控制面板的程序卸载注册，不设置的话则不在程序卸载列表显示
    mytable["app_name"] = "调试软件"                                  -- 软件名称 快捷方式也会使用这个名称
    mytable["main_bin"] = "DbgView.exe"                              -- 程序入口文件名
    mytable["install_dir"] = "MyDbgView"                              -- 默认安装路径 会自动加上前缀，例如： C:\\Program Files\\MyDbgView
    mytable["title"] = "调试软件安装程序"                                    -- 标题
    mytable["master_color"] = "#ff2f73e7"                           -- 主颜色、master hot、master pushed、master disable
    mytable["bk_color"] = "#fff7f7f7"                               -- 窗口背景颜色
    mytable["btn_master_hover_color"] = "#ff5f97f3"
    mytable["btn_master_pushed_color"] = "#ff3c79ae"
    mytable["btn_master_text_color"] = "#ffffffff"
    mytable["btn_master_disable_color"] = "#ffe4e4e4"
    mytable["btn_master_disable_textcolor"] = "#ffa7a6aa"
    mytable["text_color"] = "#ff999999"
    mytable["text_link_color"] = "#ff576b95"
    mytable["btn_plain_hover_color"] = "#ff5f97f3"
    mytable["btn_plain_pushed_color"] = "#ff3c79ae"
    mytable["btn_plain_disable_color"] = "#ffe4e4e4"
    mytable["btn_plain_text_color"] = "#ff000000"
    mytable["btn_plain_disable_textcolor"] = "#ffa7a6aa"
    mytable["btn_plain_border_color"] = "#ffe7e7e7"
    mytable["edit_plaint_border_color"] = "#ffe7e7e7"
    mytable["edit_plaint_hover_border_color"] = "#ff5f97f3"
    mytable["service_agreement_url"] = "https://baidu.com"      -- 服务协议跳转链接
    mytable["service_agreement_accept_text"] = "我已阅读并同意"
    mytable["service_agreement_text"] = "服务协议"                   -- 服务协议文本
    mytable["install_button_text"] = "安装"                      -- 安装按钮文本
    mytable["run_text"] = "开始使用"
    mytable["app_size"] = "48MB"
    mytable["logo"] = "logo.png"                                    -- 设置安装包LOGO
    mytable["version"] = "0.0.1.0"
    mytable["publisher"] = "软件公司名称"
    add_opts(mytable)
end


function init()
    -- 锁定安装目录
    -- enable_install_path(0)
    local major, minor, build_number = get_os_version()
    logi("init() 当前操作系统版本：%d.%d.%d", major, minor, build_number)

    -- 示例只支持win10
    if major < 10 then
        local msg = "不支持的windows版本！"
        show_dlg(msg)
        error("os not support")
        return
    end
end

-- 释放文件前调用(可选)
function before_release()
    -- 安装前可能需要停止的进程或服务
    -- 判断是否有服务运行
    local service_status_code = query_service_status("testsvc1")
    if service_status_code ~= 0xff then
        local ret = show_dlg("当前已安装，是否更新？")
        if ret ~= 1 then
            error("用户没有同意更新")
            return
        end
    end

    local processes = get_process()
    for i, p in ipairs(processes) do
        logi("PID: %d, PPID: %d, EXE: %s", p.pid, p.ppid, p.exe)
    end

    -- 如果有
    -- 卸载服务
    -- 停止进程
    -- 卸载驱动
end

function install()
    -- 自己实现想要的操作
    -- 如果安装服务
    -- 等等

    -- 快速添加快捷方式等
    install_all()
end

-- 立即运行
function run()
    local bin_path = get_string("install_dir") .. "\\" .. get_string("main_bin")
    logi("bin: %s", bin_path)
    create_process(bin_path)
    exit_process()
end

function uninstall()
    logi("uninstall begin")
    -- 自己实现
    -- 注册表清理之类的操作
    -- 停止进程
    -- 停止服务
    -- 卸载驱动
    -- 等等


    -- 快速删除快捷方式等
    uninstall_all()
end
