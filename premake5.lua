workspace "BlackLua"
    configurations { "Debug", "Release" }

    project "BlackLua"
        language "C++"
        cppdialect "C++20"
        kind "StaticLib"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files {"src/**.cpp", "src/**.hpp"}

        includedirs { "src/black_lua" }

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

        includedirs { "src/black_lua" }

        links { "BlackLua", "lua" }

        filter "configurations:Debug"
            symbols "On"

        filter "configurations:Release"
            optimize "On"

    project "lua"
        language "C"
        cdialect "C89"
        kind "StaticLib"
        staticruntime "On"
        
        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")
        
        files { "src/vendor/lua/src/**.c", "src/vendor/lua/src/**.h" }
        removefiles { "src/vendor/lua/src/lua.c" }
        
        includedirs { "src/vendor/lua/src/" }