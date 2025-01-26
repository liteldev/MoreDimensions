add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("levilamina 1.0.0")
add_requires("levibuildscript")
add_requires("snappy 1.2.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Enable tests")

target("more-dimensions")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
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
        "levilamina",
        "snappy"
    )
    add_shflags(
        "/DELAYLOAD:bedrock_server.dll"
    )
    set_exceptions("none")
    set_kind("shared")
    set_symbols("debug")
    set_languages("c++20")

    if has_config("tests") then
        add_files("src/test/TestCustomDimension.cpp",
                  "src/test/generator/flat-gen-village/**.cpp")
    end
