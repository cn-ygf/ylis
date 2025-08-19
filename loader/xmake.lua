add_rules("mode.debug", "mode.release")

add_requires("lua", {configs = {
    shared = false,
    vs_runtime = vs_runtime_cfg
}, debug = is_mode("debug")})

add_requires("libhv", {
    configs = {
        shared = false,
        ssl = true,
        protocol = true,
        http = true,
        http_server = true,
        http_client = true,
        websocket = true,
        runtimes = vs_runtime_cfg
    },
    debug = is_mode("debug")
})

add_requires("minizip", {configs = {
    shared = false,
    vs_runtime = vs_runtime_cfg
}, debug = is_mode("debug")})

add_includedirs("./", "../")
add_includedirs("../utils/")

target("loader")
    set_kind("binary")
    set_languages("c11", "c++17")
    add_rules("utils.bin2c", {extensions = {".lua"}})
    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_defines("WIN32_LEAN_AND_MEAN")
    add_defines("_UNICODE", "UNICODE", "_WINDOWS")
    add_defines("HV_STATICLIB")
    set_pcxxheader("src/pch.h")
    add_files("src/*.cpp")
    add_files("src/lualib/*.cpp", {header = false})
    add_files("../utils/utils.cpp", {header = false})
    add_files("../utils/utils_res.cpp", {header = false})
    add_files("src/scripts/lualib.lua")
    add_files("app.manifest")
    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_cxxflags("/utf-8", {force = true})
    add_deps("base", "duilib")
    add_links("User32", "kernel32", "Imm32", "Comctl32", "Msimg32")
    add_links("ole32", "shell32", "gdi32", "gdiplus")
    add_packages("lua")
    add_packages("libhv")
    add_packages("minizip")
    add_cxxflags("/utf-8", {force = true})
    add_cxflags("/wd4838", {force = true})
    add_cxxflags("/wd4838", {force = true})
    add_cxflags("/wd4828", {force = true})
    add_cxxflags("/wd4828", {force = true})
    after_build(function(target) 
        local outfile = target:targetfile()
        local dest = path.join("ylis", "res", "loader.bin")
        os.cp(outfile, dest)
        print("Copied to " .. dest)
    end)

target("lua_test")
    set_kind("binary")
    set_languages("c11", "c++17")
    add_rules("utils.bin2c", {extensions = {".lua"}})
    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_defines("WIN32_LEAN_AND_MEAN")
    add_defines("_UNICODE", "UNICODE", "_WINDOWS")
    add_defines("HV_STATICLIB")
    add_cxxflags("/utf-8", {force = true})
    add_files("src/tests/lua_test.cpp");
    add_files("src/lualib/*.cpp", {header = false})
    add_files("../utils/utils.cpp", {header = false})
    add_files("src/scripts/*.lua")
    add_deps("base")
    add_links("User32", "kernel32", "Imm32", "Comctl32", "Msimg32")
    add_links("ole32", "Shell32", "gdi32", "gdiplus")
    add_packages("lua")
    add_packages("libhv")

target("ui_test")
    set_kind("binary")
    set_languages("c11", "c++17")
    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_defines("WIN32_LEAN_AND_MEAN")
    add_defines("_UNICODE", "UNICODE")
    add_defines("HV_STATICLIB")
    add_files("src/tests/ui_test.cpp", {header = false})
    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_cxxflags("/utf-8", {force = true})
    add_deps("base", "duilib")
    add_links("User32", "kernel32", "Imm32", "Comctl32", "Msimg32")
    add_links("ole32", "shell32", "gdi32", "gdiplus","Advapi32", "Shlwapi")
    add_cxxflags("/utf-8", {force = true})
    add_cxflags("/wd4838", {force = true})
    add_cxxflags("/wd4838", {force = true})
    add_cxflags("/wd4828", {force = true})
    add_cxxflags("/wd4828", {force = true})