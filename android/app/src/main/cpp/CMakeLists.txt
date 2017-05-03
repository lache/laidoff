cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
add_library(app-glue STATIC
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DdDOUBLE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DdDOUBLE")

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

file(GLOB LAIDOFF_LIB
    "../../../../../src/*.h"
    "../../../../../src/*.c"
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
)

target_include_directories(native-activity PRIVATE
                           ${ANDROID_NDK}/sources/android/native_app_glue
                           ../../../../../glfw-3.2.1/deps
                           ../../../../../src
                           ../../../../../lua-5.3.4
                           ../../../../../ode/include
                           ../../../../../ode/ou/include
                           ../../../../../ode/ode/src)

# add lib dependencies
target_link_libraries(native-activity
                      android
                      app-glue
                      EGL
                      GLESv2
                      log)
