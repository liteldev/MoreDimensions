add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("levilamina 0.13.4")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Enable tests")

target("more-dimensions")
    add_cxflags(
        "/EHa",
        "/utf-8"
    )
    add_defines(
        "MORE_DIMENSIONS_EXPORTS",
        "NOMINMAX",
        "UNICODE"
    )
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
    add_packages(
        "levilamina"
    )
    add_shflags(
        "/DELAYLOAD:bedrock_server.dll"
    )
    set_exceptions("none")
    set_kind("shared")
    set_symbols("debug")
    set_languages("cxx20")

    if has_config("tests") then
        add_files("src/test/TestCustomDimension.cpp",
                  "src/test/generator/flat-gen-village/**.cpp")
    end

    after_build(function (target)
        local plugin_packer = import("scripts.after_build")

        local plugin_define = {
            pluginName = target:name(),
            pluginFile = path.filename(target:targetfile()),
        }
        
        plugin_packer.pack_plugin(target,plugin_define)
    end)
