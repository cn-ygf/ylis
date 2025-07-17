-- 添加开机启动
function add_auto_run()
    local app_name = get_string("app_name")
    local main_bin = get_string("main_bin")
    local install_dir = get_string("install_dir")
    if app_name ~= "" and main_bin ~= "" and install_dir ~= "" then
        local bin_path = install_dir .. "\\" .. main_bin
        set_reg_string("HKLM", "Software\\Microsoft\\Windows\\CurrentVersion\\Run", app_name, bin_path)
    end
end

-- 删除开机启动
function remove_auto_run()
    local app_name = get_string("app_name")
    if app_name ~= "" then
        del_reg_value("HKLM", "Software\\Microsoft\\Windows\\CurrentVersion\\Run", app_name)
    end
end

-- 创建桌面快捷方式
function add_desktop_link()
    local app_name = get_string("app_name")
    local main_bin = get_string("main_bin")
    local install_dir = get_string("install_dir")
    if app_name ~= "" and main_bin ~= "" and install_dir ~= "" then
        local bin_path = install_dir .. "\\" .. main_bin
        create_shortcut(app_name, bin_path)
    end
end

-- 移除桌面快捷方式
function remove_desktop_link()
    local app_name = get_string("app_name")
    if app_name ~= "" then
        remove_shortcut(app_name)
    end
end

-- 创建开始菜单
function add_menu_link()
    local app_name = get_string("app_name")
    local main_bin = get_string("main_bin")
    local install_dir = get_string("install_dir")

    if app_name ~= "" and main_bin ~= "" and install_dir ~= "" then
        local bin_path = install_dir .. "\\" .. main_bin
        -- 调用 create_startmenu_shortcut，传入 app_name, bin_path, 额外参数均用默认空字符串，最后传入子目录
        create_startmenu_shortcut(app_name, bin_path, "", "", "")
    end
end

-- 删除开始菜单
function remove_menu_link()
    local app_name = get_string("app_name")
    if app_name ~= "" then
        remove_startmenu_shortcut(app_name)
    end
end

-- 添加卸载注册表配置
function add_uninstall_info()
    local guid = get_string("guid")
    if guid == "" then
        return
    end
    local app_name = get_string("app_name")
    local main_bin = get_string("main_bin")
    local install_dir = get_string("install_dir")
    local bin_path = install_dir .. "\\" .. main_bin

    local reg_base = "HKLM"
    local reg_path = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" .. guid

    local version = get_string("version")
    local publisher = get_string("publisher") 
    local uninstall_cmd = install_dir .. "\\uninstall.exe"
    local date = os.date("%Y%m%d")  -- 格式: 20250523

    -- 设置注册表值
    set_reg_string(reg_base, reg_path, "DisplayName", app_name)
    set_reg_string(reg_base, reg_path, "DisplayIcon", bin_path .. ",0")
    set_reg_string(reg_base, reg_path, "DisplayVersion", version)
    set_reg_string(reg_base, reg_path, "Publisher", publisher)
    set_reg_string(reg_base, reg_path, "InstallLocation", install_dir)
    set_reg_string(reg_base, reg_path, "UninstallString", uninstall_cmd)
    set_reg_string(reg_base, reg_path, "InstallDate", date)

    -- 可选
    set_reg_dword(reg_base, reg_path, "NoModify", 1)
    set_reg_dword(reg_base, reg_path, "NoRepair", 1)
end

-- 删除卸载注册表配置
function remove_uninstall_info()
    local guid = get_string("guid")
    if guid == "" then
        return
    end
    local reg_base = "HKLM"
    local reg_path = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" .. guid
    del_reg(reg_base, reg_path)
end

function install_all()
    add_auto_run()
    add_desktop_link()
    add_menu_link()
    add_uninstall_info()
end

function uninstall_all()
    remove_auto_run()
    remove_desktop_link()
    remove_menu_link()
    remove_uninstall_info()
end

