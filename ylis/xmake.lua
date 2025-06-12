add_rules("mode.debug", "mode.release")

add_requires("lua", {configs = {
    shared = false,
    vs_runtime = vs_runtime_cfg
}, debug = is_mode("debug")})

add_requires("fmt", {configs = {
    shared = false,
    vs_runtime = vs_runtime_cfg
}, debug = is_mode("debug")})

add_requires("libhv", {configs = {
    shared = false,
    ssl = true,
    http = true,
    websocket = true,
    vs_runtime = vs_runtime_cfg
}, debug = is_mode("debug")})

add_requires("minizip", {configs = {
    shared = false,
    vs_runtime = vs_runtime_cfg
}, debug = is_mode("debug")})

add_requires("tabulate")

add_includedirs("./", "../")

target("ylis")
    set_kind("binary")
    set_languages("c11", "c++17")
    add_rules("utils.bin2c", {extensions = {".bin",".xml"}})
    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_defines("WIN32_LEAN_AND_MEAN")
    add_defines("HV_STATICLIB")
    add_defines("_UNICODE", "UNICODE")
    add_cxxflags("/utf-8", {force = true})
    add_files("src/*.cpp")
    add_files("src/lualib/*.cpp", {header = false})
    add_files("../utils/utils.cpp", {header = false})
    add_files("../utils/utils_zip.cpp", {header = false})
    add_files("../utils/utils_res.cpp", {header = false})
    add_files("res/skin.bin")
    add_files("res/loader.bin")
    add_files("res/global.xml")
    add_files("res/main.xml")
    add_files("res/dlg.xml")
    add_files("res/uninstall.xml")
    add_deps("base")
    add_links("User32", "kernel32", "Imm32", "Comctl32", "Msimg32")
    add_links("ole32", "shell32", "gdi32", "gdiplus")
    add_packages("lua","zlib")
    add_packages("libhv")
    add_packages("tabulate")
    add_packages("minizip")
    add_packages("fmt")
    add_cxxflags("/utf-8", {force = true})
    add_cxflags("/wd4838", {force = true})  -- C 代码
    add_cxxflags("/wd4838", {force = true}) -- C++ 代码
    add_cxflags("/wd4828", {force = true})  -- C 代码
    add_cxxflags("/wd4828", {force = true}) -- C++ 代码
    add_deps("base")
    add_deps("loader")
    