workspace "BlackLua"
    configurations { "Debug", "Release" }

    project "BlackLua"
        language "C++"
        cppdialect "C++20"
        kind "StaticLib"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files {"src/**.cpp", "src/**.hpp"}

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

        includedirs { "src/" }

        links { "BlackLua" }

        filter "configurations:Debug"
            symbols "On"

        filter "configurations:Release"
            optimize "On"
