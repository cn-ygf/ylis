add_rules("mode.debug", "mode.release")

vs_runtime_cfg = is_mode("debug") and "MTd" or "MT"
set_runtimes(vs_runtime_cfg)


includes("base/xmake.lua")
includes("duilib/xmake.lua")
includes("loader/xmake.lua")
includes("ylis/xmake.lua")