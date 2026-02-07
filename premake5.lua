workspace "BlackLua"
    configurations { "Debug", "Release" }

    project "BlackLua"
        language "C++"
        cppdialect "C++20"
        kind "StaticLib"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files {"src/**.cpp", "src/**.hpp"}

        includedirs { "src/black_lua", "src/vendor/fmt/include/" }

        filter "configurations:Debug"
            symbols "On"

        filter "configurations:Release"
            optimize "On"

    project "BlackLuaExample"
        language "C++"
        cppdialect "C++20"
        kind "ConsoleApp"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files { "example/example.cpp" }

        includedirs { "src/black_lua", "src/vendor/fmt/include/" }

        links { "BlackLua", "fmt" }

        filter "configurations:Debug"
            symbols "On"

        filter "configurations:Release"
            optimize "On"

    project "fmt"
        language "C++"
        cppdialect "C++20"
        kind "StaticLib"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files { "src/vendor/fmt/src/format.cc", "src/vendor/fmt/src/os.cc" }

        includedirs { "src/vendor/fmt/include/" }

        defines { "FMT_UNICODE=0" }

        filter "configurations:Debug"
            symbols "On"

        filter "configurations:Release"
            optimize "On"