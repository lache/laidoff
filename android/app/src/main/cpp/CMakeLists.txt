cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
add_library(app-glue STATIC
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

set(LUA_LIB
../../../../../lua-5.3.4/ldblib.c
../../../../../lua-5.3.4/ldebug.c
../../../../../lua-5.3.4/ldebug.h
../../../../../lua-5.3.4/ldo.c
../../../../../lua-5.3.4/ldo.h
../../../../../lua-5.3.4/ldump.c
../../../../../lua-5.3.4/lfunc.c
../../../../../lua-5.3.4/lfunc.h
../../../../../lua-5.3.4/lgc.c
../../../../../lua-5.3.4/lgc.h
../../../../../lua-5.3.4/linit.c
../../../../../lua-5.3.4/liolib.c
../../../../../lua-5.3.4/llex.c
../../../../../lua-5.3.4/llex.h
../../../../../lua-5.3.4/llimits.h
../../../../../lua-5.3.4/lmathlib.c
../../../../../lua-5.3.4/lmem.c
../../../../../lua-5.3.4/lmem.h
../../../../../lua-5.3.4/loadlib.c
../../../../../lua-5.3.4/lobject.c
../../../../../lua-5.3.4/lobject.h
../../../../../lua-5.3.4/lopcodes.c
../../../../../lua-5.3.4/lopcodes.h
../../../../../lua-5.3.4/loslib.c
../../../../../lua-5.3.4/lparser.c
../../../../../lua-5.3.4/lparser.h
../../../../../lua-5.3.4/lprefix.h
../../../../../lua-5.3.4/lstate.c
../../../../../lua-5.3.4/lstate.h
../../../../../lua-5.3.4/lstring.c
../../../../../lua-5.3.4/lstring.h
../../../../../lua-5.3.4/lstrlib.c
../../../../../lua-5.3.4/ltable.c
../../../../../lua-5.3.4/ltable.h
../../../../../lua-5.3.4/ltablib.c
../../../../../lua-5.3.4/ltm.c
../../../../../lua-5.3.4/ltm.h
../../../../../lua-5.3.4/lua.c
../../../../../lua-5.3.4/lua.h
../../../../../lua-5.3.4/lua.hpp
../../../../../lua-5.3.4/luac.c
../../../../../lua-5.3.4/luaconf.h
../../../../../lua-5.3.4/lualib.h
../../../../../lua-5.3.4/lundump.c
../../../../../lua-5.3.4/lundump.h
../../../../../lua-5.3.4/lutf8lib.c
../../../../../lua-5.3.4/lvm.c
../../../../../lua-5.3.4/lvm.h
../../../../../lua-5.3.4/lzio.c
../../../../../lua-5.3.4/lzio.h
../../../../../lua-5.3.4/lapi.c
../../../../../lua-5.3.4/lapi.h
../../../../../lua-5.3.4/lauxlib.c
../../../../../lua-5.3.4/lauxlib.h
../../../../../lua-5.3.4/lbaselib.c
../../../../../lua-5.3.4/lbitlib.c
../../../../../lua-5.3.4/lcode.c
../../../../../lua-5.3.4/lcode.h
../../../../../lua-5.3.4/lcorolib.c
../../../../../lua-5.3.4/lctype.c
../../../../../lua-5.3.4/lctype.h
)

add_library(native-activity SHARED
            main.cpp
            image.cpp
            sound.cpp
            file.c
            net.c
            ../../../../../src/lwsimpleanim.c
            ../../../../../src/battle.c
            ../../../../../src/battlelogic.c
            ../../../../../src/battle_result.c
            ../../../../../src/dialog.c
            ../../../../../src/lwenemy.c
            ../../../../../src/font.c
            ../../../../../src/ktx.c
            ../../../../../src/laidoff.c
            ../../../../../src/loadobj.c
            ../../../../../src/render_battle.c
            ../../../../../src/render_battle_result.c
            ../../../../../src/lwuialign.c
            ../../../../../src/lwuidim.c
            ../../../../../src/render_dialog.c
            ../../../../../src/render_field.c
            ../../../../../src/render_text_block.c
            ../../../../../src/script.c
            ../../../../../src/unicode.c
            ../../../../../src/tex.c
            ../../../../../src/render_font_test.c
            ../../../../../src/render_solid.c
            ../../../../../src/render_admin.c
            ../../../../../src/lwtimepoint.c
            ../../../../../src/input.c
            ../../../../../src/field.c
            ../../../../../src/lwbattlecreature.c
            ${LUA_LIB}
            )

target_include_directories(native-activity PRIVATE
                           ${ANDROID_NDK}/sources/android/native_app_glue
                           ../../../../../glfw-3.2.1/deps
                           ../../../../../src
                           ../../../../../lua-5.3.4)

# add lib dependencies
target_link_libraries(native-activity
                      android
                      app-glue
                      EGL
                      GLESv2
                      log)
