workspace "rec"
	configurations {"debug", "release"}
project "rec"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	targetname "rec"
	files {"**.cpp", "**.h"}
	objdir "obj/%{cfg.buildcfg}"
	targetdir "bin/%{cfg.buildcfg}"
	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"
		optimize "Debug"
	filter "configurations:release"
		defines { "NDEBUG" }
		optimize "On"
	filter { "system:linux", "action:gmake2" }
		buildoptions {"-Wnrvo"}