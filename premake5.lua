workspace "Aria"
    configurations { "Debug", "Release" }

    project "AriaLib"
        language "C++"
        cppdialect "C++20"
        kind "StaticLib"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files {"src/aria/**.cpp", "src/aria/**.hpp"}

        includedirs { "src/", "src/vendor/fmt/include/" }

        filter "configurations:Debug"
            symbols "On"

        filter "configurations:Release"
            optimize "On"

    project "AriaTest"
        language "C++"
        cppdialect "C++20"
        kind "ConsoleApp"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files { "tests/**.cpp", "tests/**.hpp" }

        includedirs { "tests/", "src/", "src/vendor/catch2/", "src/vendor/fmt/include/" }

        links { "AriaLib", "fmt" }

        filter "configurations:Debug"
            symbols "On"

        filter "configurations:Release"
            optimize "On"

    project "Aria"
        language "C++"
        cppdialect "C++20"
        kind "ConsoleApp"

        targetdir("build/bin/%{cfg.buildcfg}/")
        objdir("build/obj/%{cfg.buildcfg}/")

        files { "frontend/aria.cpp" }

        includedirs { "src/", "src/vendor/fmt/include/" }

        links { "AriaLib", "fmt" }

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
