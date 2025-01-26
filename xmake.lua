add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

if is_config("target_type", "server") then
    add_requires("levilamina 1.0.0", {configs = {target_type = "server"}})
else
    add_requires("levilamina 1.0.0", {configs = {target_type = "client"}})
end

add_requires("levibuildscript")
add_requires("snappy 1.2.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Enable tests")

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

target("more-dimensions")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE", "MORE_DIMENSIONS_EXPORTS")
    add_packages("levilamina", "snappy")
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_files(
        "src/more_dimensions/**.cpp"
    )
    add_includedirs(
        "src"
    )
    add_headerfiles(
        "src/(more_dimensions/api/**.h)",
        "src/(more_dimensions/core/Macros.h)"
    )
    if has_config("tests") then
        add_files("src/test/TestCustomDimension.cpp",
                  "src/test/generator/flat-gen-village/**.cpp")
    end