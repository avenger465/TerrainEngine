workspace "TerrainEngine"
	architecture "x64"
	 
	configurations
	{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["ImGui"] = "Engine/External/ImGui"
IncludeDir["ImGuiBackends"] = "Engine/External/ImGui/backends"
IncludeDir["Assimp"] = "Engine/External/assimp/include"
IncludeDir["DirectX"] = "Engine/External/DirectXTK"


LibDir = {}
LibDir["assimp"] = "Engine/External/assimp/lib/xSixtyFour"
LibDir["DirectXTK"] = "Engine/External/DirectXTK/%{cfg.buildcfg}"

include "Engine/External/ImGui"

project "Engine"
	location "Engine"
	kind "WindowedApp"
	language "C++"
	staticruntime "Off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "tepch.h"
	pchsource "Engine/Src/tepch.cpp"

	files
	{
		"%{prj.name}/Src/**.cpp",
		"%{prj.name}/Src/**.h",
		"%{prj.name}/External/ImGui/backends/imgui_impl_dx11.h",
		"%{prj.name}/External/ImGui/backends/imgui_impl_dx11.cpp",
		"%{prj.name}/External/ImGui/backends/imgui_impl_win32.h",
		"%{prj.name}/External/ImGui/backends/imgui_impl_win32.cpp"

	}

		
	includedirs
	{
		"%{prj.name}/Src",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuiBackends}",
		"%{IncludeDir.DirectX}"
	}

	libdirs
	{
		"%{LibDir.assimp}",
		"%{LibDir.DirectXTK}"
	}

	links
	{
		"ImGui",
		"d3d11.lib",
		"assimp-vc140-mt.lib",
		"DirectXTK.lib",
		"d3dcompiler.lib",
		"winmm.lib"
	}

	files("Engine/Src/Shaders/*.hlsl")
	shadermodel("5.0")

	local shader_dir = "Src/Shaders/"
	
	filter("files:**.hlsl")
		--flags("ExcludeFromBuild")
		shaderobjectfileoutput(shader_dir.."%{file.basename}"..".cso")
	
	filter("files:**_ps.hlsl")
		--removeflags("ExcludeFromBuild")
		shadertype("Pixel")

	filter("files:**_vs.hlsl")
		--removeflags("ExcludeFromBuild")
		shadertype("Vertex")
		shaderoptions({"/WX"})

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"PG_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "PG_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "PG_Release"
		runtime "Release"
		optimize "On"	
