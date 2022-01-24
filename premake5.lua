workspace "TerrainEngine"
	architecture "x64"
	 
	configurations
	{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["ImGui"] = "Engine/External/GUI"
IncludeDir["ImGuiBackends"] = "Engine/External/GUI/backends"
IncludeDir["Assimp"] = "Engine/External/assimp/include"
IncludeDir["DirectX"] = "Engine/External/DirectXTK"
IncludeDir["TinyXML2"] = "Engine/External/TinyXML2"


LibDir = {}
LibDir["assimp"] = "Engine/External/assimp/lib/xSixtyFour"
LibDir["DirectXTK"] = "Engine/External/DirectXTK/%{cfg.buildcfg}"

include "Engine/External/GUI"

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
		"%{prj.name}/External/GUI/backends/imgui_impl_dx11.h",
		"%{prj.name}/External/GUI/backends/imgui_impl_dx11.cpp",
		"%{prj.name}/External/GUI/backends/imgui_impl_win32.h",
		"%{prj.name}/External/GUI/backends/imgui_impl_win32.cpp",
		"%{prj.name}/Src/Shaders/Common.hlsli"

	}

		
	includedirs
	{
		"%{prj.name}/Src",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuiBackends}",
		"%{IncludeDir.DirectX}",
		"%{IncludeDir.TinyXML2}"
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
	
	files("Engine/External/GUI/backends/imgui_impl_win32.cpp")
	flags("NoPCH")

	files("Engine/External/GUI/backends/imgui_impl_dx11.cpp")
	flags("NoPCH")

	files("Engine/Src/Shaders/*.hlsl")
	shadermodel("5.0")

	local shader_dir = "Src/Shaders/"
	
	filter("files:**.hlsl")
		--flags("ExcludeFromBuild")
		shaderobjectfileoutput(shader_dir.."%{file.basename}"..".cso")
	
	filter("files:**_ps.hlsl")
		--removeflags("ExcludeFromBuild")
		shadertype("Pixel")

	filter("files:**_gs.hlsl")
		--removeflags("ExcludeFromBuild")
		shadertype("Geometry")

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
