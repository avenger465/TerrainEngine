workspace "Terrain"
	architecture "x64"
	startproject "TerrainGeneration"
	 
	configurations
	{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["ImGui"]		    = "DirectXEngine/External/GUI"
IncludeDir["ImGuiBackends"] = "DirectXEngine/External/GUI/backends"
IncludeDir["Assimp"]		= "DirectXEngine/External/assimp/include"
IncludeDir["DirectX"]		= "DirectXEngine/External/DirectXTK"
IncludeDir["TinyXML2"]		= "DirectXEngine/External/TinyXML2"


LibDir = {}
LibDir["assimp"]	= "DirectXEngine/External/assimp/lib/xSixtyFour"
LibDir["DirectXTK"] = "DirectXEngine/External/DirectXTK/%{cfg.buildcfg}"

include "DirectXEngine/External/GUI"

project "DirectXEngine"
	location "DirectXEngine"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "tepch.h"
	pchsource "DirectXEngine/Src/tepch.cpp"

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
		"DirectXTex.lib",
		"d3dcompiler.lib",
		"winmm.lib"
	}
	
	files("DirectXEngine/External/GUI/backends/imgui_impl_win32.cpp")
	flags("NoPCH")

	files("DirectXEngine/External/GUI/backends/imgui_impl_dx11.cpp")
	flags("NoPCH")

	files("DirectXEngine/Src/Shaders/*.hlsl")
	shadermodel("5.0")

	local shader_dir = "Src/Shaders/"
	
	filter("files:**.hlsl")
		shaderobjectfileoutput(shader_dir.."%{file.basename}"..".cso")
	
	filter("files:**_ps.hlsl")
		shadertype("Pixel")

	filter("files:**_gs.hlsl")
		shadertype("Geometry")

	filter("files:**_vs.hlsl")
		shadertype("Vertex")
		shaderoptions({"/WX"})

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"DXE_PLATFORM_WINDOWS",
		}

	filter "configurations:Debug"
		defines "DXE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "DXE_Release"
		runtime "Release"
		optimize "on"	