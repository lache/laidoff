cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
add_library(app-glue STATIC
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DdDOUBLE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DdDOUBLE")

# Android Studio에서 CMake 빌드 오류나는 부분 수정
set(ZMQ_HAVE_EVENTFD_CLOEXEC_EXITCODE 0)
set(ZMQ_HAVE_SO_KEEPALIVE_EXITCODE 0)
set(ZMQ_HAVE_TCP_KEEPCNT_EXITCODE 0)
set(ZMQ_HAVE_TCP_KEEPIDLE_EXITCODE 0)
set(ZMQ_HAVE_TCP_KEEPINTVL_EXITCODE 0)
set(ZMQ_HAVE_SOCK_CLOEXEC 0)
set(ZMQ_HAVE_TIPC 0)

# ZeroMQ 라이브러리 중 테스트 프로젝트는 빌드하지 않는다.
set (ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(../../../../../zeromq-4.2.1 zeromq)

# ZeroMQ 프로젝트는 ZeroMQ 솔루션 디렉토리에 모은다.
set_target_properties(libzmq-static PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

include_directories(../../../../../zeromq-4.2.1/include)

# CZMQ CMake 의존성 추가

add_subdirectory(../../../../../czmq czmq)
include_directories(../../../../../czmq/include)

# ZeroMQ 라이브러리 중 사용할 것만 제외하고는 빌드되지 않도록 한다.
set_target_properties(czmq czmq_selftest zmakecert zsp PROPERTIES FOLDER "ZeroMQ")
# ZeroMQ 프로젝트는 ZeroMQ 솔루션 디렉토리에 모은다.
set_target_properties(czmq_selftest zmakecert zsp PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

include_directories(czmq/include)



file(GLOB LUA_LIB
    "../../../../../lua-5.3.4/*.h"
    "../../../../../lua-5.3.4/*.c"
)

file(GLOB ODE_LIB
    "../../../../../ode/ode/src/*.h"
    "../../../../../ode/ode/src/*.c"
    "../../../../../ode/ode/src/*.cpp"
    "../../../../../ode/ode/src/joints/*.h"
    "../../../../../ode/ode/src/joints/*.cpp"
    "../../../../../ode/ou/src/ou/*.cpp"
    "../../../../../ode/include/ode/*.h"
    "../../../../../ode/ou/include/ou/*.h"
)

file(GLOB DETOUR_LIB
    "../../../../../recastnavigation/Detour/Include/*.h"
    "../../../../../recastnavigation/Detour/Source/*.cpp")

file(GLOB LAIDOFF_LIB
    "../../../../../src/*.h"
    "../../../../../src/*.c"
    "../../../../../src/*.cpp"
)

source_group(Lua FILES ${LUA_LIB})

source_group(Ode FILES ${ODE_LIB})

add_library(native-activity SHARED
    image.cpp
    main.cpp
    sound.cpp
    file.c
    net.c
    ${LAIDOFF_LIB}
    ${LUA_LIB}
    ${ODE_LIB}
    ${DETOUR_LIB}
)

target_include_directories(native-activity PRIVATE
                           ${ANDROID_NDK}/sources/android/native_app_glue
                           ../../../../../glfw-3.2.1/deps
                           ../../../../../src
                           ../../../../../lua-5.3.4
                           ../../../../../ode/include
                           ../../../../../ode/ou/include
                           ../../../../../ode/ode/src
                           ../../../../../recastnavigation/Detour/Include)

# add lib dependencies
target_link_libraries(native-activity
                      android
                      app-glue
                      EGL
                      GLESv2
                      log
                      libzmq
                      czmq)

					  
					  