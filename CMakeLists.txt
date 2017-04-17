cmake_minimum_required(VERSION 3.6)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw-3.2.1)

include_directories(${glfw_INCLUDE_DIRS})

include_directories("${GLFW_SOURCE_DIR}/deps")

include_directories("src")

include_directories("lua-5.3.4")

#include_directories(libpng-1.6.28)

#link_directories(libpng-1.6.28/projects/vstudio/Release)

include_directories(/opt/vc/include /opt/vc/include/interface/vcos/pthreads /opt/vc/include/interface/vmcs_host/linux /opt/vc/src/hello_pi/libs/ilclient /opt/vc/src/hello_pi/libs/vgfont)

include(CheckIncludeFile)
CHECK_INCLUDE_FILE(/opt/vc/include/bcm_host.h BCM_HOST_AVAILABLE)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (WIN32)
    set(ICON win32/laidoff.rc)
	set(WINCODEC Windowscodecs) # Windows Imaging Codecs
	set(WINDSHOW Strmiids) # DirectShow
	set(WINOPENGL32 Opengl32)
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

set(LUA_LIB
lua-5.3.4/ldblib.c
lua-5.3.4/ldebug.c
lua-5.3.4/ldebug.h
lua-5.3.4/ldo.c
lua-5.3.4/ldo.h
lua-5.3.4/ldump.c
lua-5.3.4/lfunc.c
lua-5.3.4/lfunc.h
lua-5.3.4/lgc.c
lua-5.3.4/lgc.h
lua-5.3.4/linit.c
lua-5.3.4/liolib.c
lua-5.3.4/llex.c
lua-5.3.4/llex.h
lua-5.3.4/llimits.h
lua-5.3.4/lmathlib.c
lua-5.3.4/lmem.c
lua-5.3.4/lmem.h
lua-5.3.4/loadlib.c
lua-5.3.4/lobject.c
lua-5.3.4/lobject.h
lua-5.3.4/lopcodes.c
lua-5.3.4/lopcodes.h
lua-5.3.4/loslib.c
lua-5.3.4/lparser.c
lua-5.3.4/lparser.h
lua-5.3.4/lprefix.h
lua-5.3.4/lstate.c
lua-5.3.4/lstate.h
lua-5.3.4/lstring.c
lua-5.3.4/lstring.h
lua-5.3.4/lstrlib.c
lua-5.3.4/ltable.c
lua-5.3.4/ltable.h
lua-5.3.4/ltablib.c
lua-5.3.4/ltm.c
lua-5.3.4/ltm.h
lua-5.3.4/lua.c
lua-5.3.4/lua.h
lua-5.3.4/lua.hpp
lua-5.3.4/luac.c
lua-5.3.4/luaconf.h
lua-5.3.4/lualib.h
lua-5.3.4/lundump.c
lua-5.3.4/lundump.h
lua-5.3.4/lutf8lib.c
lua-5.3.4/lvm.c
lua-5.3.4/lvm.h
lua-5.3.4/lzio.c
lua-5.3.4/lzio.h
lua-5.3.4/lapi.c
lua-5.3.4/lapi.h
lua-5.3.4/lauxlib.c
lua-5.3.4/lauxlib.h
lua-5.3.4/lbaselib.c
lua-5.3.4/lbitlib.c
lua-5.3.4/lcode.c
lua-5.3.4/lcode.h
lua-5.3.4/lcorolib.c
lua-5.3.4/lctype.c
lua-5.3.4/lctype.h
)

source_group(Lua FILES ${LUA_LIB})

source_group(Win32 FILES ${WIN32_DEPS} ${ICON})

#add_executable(laidoff WIN32 MACOSX_BUNDLE laidoff.c ${ICON} ${GLAD})
add_executable(laidoff
MACOSX_BUNDLE
src/battle.c
src/battle.h
src/battlelogic.c
src/battlelogic.h
src/constants.h
src/dialog.c
src/dialog.h
src/etc1.h
src/file.h
src/font.c
src/font.h
src/kiwi_api.h
src/ktx.c
src/ktx.h
src/image.h
src/laidoff.c
src/laidoff.h
src/loadobj.c
src/lwatlasenum.h
src/lwatlassprite.h
src/lwattrib.h
src/lwbattlecommand.h
src/lwbattlecommandresult.h
src/lwbattlecreature.h
src/lwbitmapcontext.h
src/lwcontext.h
src/lwdamagetext.h
src/lwenemy.c
src/lwenemy.h
src/lwgl.h
src/lwlog.h
src/lwmacro.h
src/lwshader.h
src/lwsimpleanim.c
src/lwsimpleanim.h
src/lwskill.h
src/lwskilleffect.h
src/lwvbo.h
src/lwvbotype.h
src/lwtextblock.h
src/platform_detection.h
src/render_battle.c
src/render_battle.h
src/render_dialog.c
src/render_dialog.h
src/render_field.c
src/render_field.h
src/render_font_test.c
src/render_font_test.h
src/render_solid.c
src/render_solid.h
src/render_text_block.c
src/render_text_block.h
src/script.c
src/script.h
src/skill.c
src/sprite.h
src/sprite_data.h
src/tex.c
src/tex.h
src/tinyobj_loader_c.h
src/unicode.c
src/unicode.h
src/vertices.h
${LUA_LIB}
${WIN32_DEPS}
${OSX_DEPS}
${RPI_DEPS}
${ICON}
${GLAD})

target_link_libraries(laidoff glfw ${WINCODEC} ${WINDSHOW} ${WINOPENGL32})


set(WINDOWS_BINARIES laidoff)


if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(${WINDOWS_BINARIES} PROPERTIES
                          LINK_FLAGS "/ENTRY:mainCRTStartup")
elseif (APPLE)

endif()
