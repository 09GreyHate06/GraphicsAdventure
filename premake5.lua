workspace "GraphicsAdventure"
    architecture "x86_64"
    startproject "GraphicsAdventure"

    configurations
    {
        "Debug",
        "Release"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GreyDX11"]  = "GraphicsAdventure/vendor/GreyDX11/GreyDX11"
IncludeDir["ImGui"]     = "GraphicsAdventure/vendor/imgui"


include "GraphicsAdventure/vendor/GreyDX11"
include "GraphicsAdventure/vendor/imgui"

project "GraphicsAdventure"
    location "GraphicsAdventure"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{IncludeDir.GreyDX11}/src",
        "%{IncludeDir.GreyDX11}/vendor/stb_image",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.GreyDX11}/vendor/spdlog/include",
    }

    links
    {
        "GreyDX11",
        "imgui",
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines "GDX11_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "GDX11_RELEASE"
        runtime "Release"
        optimize "on"