cmake_minimum_required(VERSION 3.6)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw-3.2.1)

include_directories(${glfw_INCLUDE_DIRS})

include_directories("${GLFW_SOURCE_DIR}/deps")

include_directories("src")

include_directories("lua-5.3.4")

include_directories("ode/include")
include_directories("ode/ou/include")
include_directories("ode/ode/src")
include_directories("recastnavigation/Detour/Include")

#include_directories(libpng-1.6.28)

#link_directories(libpng-1.6.28/projects/vstudio/Release)

include_directories(/opt/vc/include /opt/vc/include/interface/vcos/pthreads /opt/vc/include/interface/vmcs_host/linux /opt/vc/src/hello_pi/libs/ilclient /opt/vc/src/hello_pi/libs/vgfont)

include(CheckIncludeFile)
CHECK_INCLUDE_FILE(/opt/vc/include/bcm_host.h BCM_HOST_AVAILABLE)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DdDOUBLE)
endif()

if (WIN32)
    set(ICON win32/laidoff.rc)
	set(WINCODEC Windowscodecs) # Windows Imaging Codecs
	set(WINDSHOW Strmiids) # DirectShow
	set(WINMM Winmm) # PlaySound
	set(WINXAUDIO2 Xaudio2) # XAudio2
	set(WINOPENGL32 Opengl32)
	set(WINSOCK2 ws2_32)
	set(WIN32_DEPS
		win32/etc1.cpp
		win32/file.c
		win32/image.cpp
		win32/key.c
		win32/main.c
		win32/main2.c
		win32/main3.c
		win32/mouse.c
		win32/sound.cpp
		win32/gl3w.c
		win32/gl3w.h
		win32/glcorearb.h
		win32/main_vulkan.cpp
		win32/stb_vorbis.c
		win32/lwxaudio2.cpp
		win32/lwxaudio2.h
		win32/net.c
		)
elseif (APPLE)
    set(ICON osx/laidoff.icns)
    set_source_files_properties(osx/laidoff.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")

    set(OSX_DEPS
            win32/etc1.cpp
            win32/file.c
            win32/image.cpp
            win32/key.c
            win32/main.c
            win32/mouse.c
            win32/sound.cpp
            osx/osx_image.m
            )
elseif (RPI)

	
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRPI -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -std=gnu11")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRPI")

	SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -L/opt/vc/src/hello_pi/libs/ilclient -L/opt/vc/src/hello_pi/libs/vgfont" )

   set(RPI_DEPS
	   win32/etc1.cpp
	   win32/file.c
	   win32/image.cpp
	   rpi/key.c
	   rpi/main.c
	   rpi/mouse.c
	   win32/sound.cpp
	   rpi/rpi_image.c
	)

endif()

set(GLAD "${GLFW_SOURCE_DIR}/deps/glad/glad.h"
         "${GLFW_SOURCE_DIR}/deps/glad.c")
set(GETOPT "${GLFW_SOURCE_DIR}/deps/getopt.h"
           "${GLFW_SOURCE_DIR}/deps/getopt.c")
set(TINYCTHREAD "${GLFW_SOURCE_DIR}/deps/tinycthread.h"
                "${GLFW_SOURCE_DIR}/deps/tinycthread.c")

file(GLOB LUA_LIB "lua-5.3.4/*.h" "lua-5.3.4/*.c")
file(GLOB ODE_LIB "ode/ode/src/*.h" "ode/ode/src/*.c" "ode/ode/src/*.cpp" "ode/ode/src/joints/*.h" "ode/ode/src/joints/*.cpp" "ode/ou/src/ou/*.cpp" "ode/include/ode/*.h" "ode/ou/include/ou/*.h")
file(GLOB LAIDOFF_LIB "src/*.h" "src/*.c" "src/*.cpp")
file(GLOB DETOUR_LIB "recastnavigation/Detour/Include/*.h" "recastnavigation/Detour/Source/*.cpp")

source_group(Lua FILES ${LUA_LIB})
source_group(Ode FILES ${ODE_LIB})
source_group(Detour FILES ${DETOUR_LIB})
source_group(Win32 FILES ${WIN32_DEPS} ${ICON})

#add_executable(laidoff WIN32 MACOSX_BUNDLE laidoff.c ${ICON} ${GLAD})
add_executable(laidoff
MACOSX_BUNDLE
${LAIDOFF_LIB}
${ODE_LIB}
${LUA_LIB}
${DETOUR_LIB}
${WIN32_DEPS}
${OSX_DEPS}
${RPI_DEPS}
${ICON}
${GLAD})

target_link_libraries(laidoff glfw ${WINCODEC} ${WINDSHOW} ${WINOPENGL32} ${WINMM} ${WINXAUDIO2} ${WINSOCK2})


set(WINDOWS_BINARIES laidoff)


if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(${WINDOWS_BINARIES} PROPERTIES
                          LINK_FLAGS "/ENTRY:mainCRTStartup")
elseif (APPLE)

endif()
