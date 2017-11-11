#pragma once

#include "platform_detection.h"

#if LW_PLATFORM_WIN32
#include <stdio.h>
#include <windows.h>
#define LOGV(...) //((void)printf(__VA_ARGS__))
#define LOGD(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGI(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGW(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGE(...) { \
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); \
CONSOLE_SCREEN_BUFFER_INFO consoleInfo; \
WORD saved_attributes; \
GetConsoleScreenBufferInfo(hConsole, &consoleInfo); \
saved_attributes = consoleInfo.wAttributes; \
SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
(void)printf(__VA_ARGS__); \
(void)printf("\n"); \
SetConsoleTextAttribute(hConsole, saved_attributes); \
}
#define LOGF(...) LOGE(__VA_ARGS__);abort()
#define LOGA(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#elif LW_PLATFORM_IOS || LW_PLATFORM_OSX
#include <stdio.h>
#define LOGV(...) //((void)printf(__VA_ARGS__))
#define LOGD(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGI(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGW(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGE(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGF(...) (void)printf(__VA_ARGS__);(void)printf("\n");abort()
#define LOGA(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#elif LW_PLATFORM_IOS_SIMULATOR || LW_PLATFORM_RPI || LW_PLATFORM_LINUX
#include <stdio.h>
#define LOGV(...) //((void)printf(__VA_ARGS__))
#define LOGD(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGI(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGW(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGE(...) printf("\x1b[33m");(void)printf(__VA_ARGS__);(void)printf("\n");printf("\x1b[0m")
#define LOGF(...) (void)printf(__VA_ARGS__);(void)printf("\n");abort()
#define LOGA(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#elif LW_PLATFORM_ANDROID
#include <android/log.h>
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "native-activity", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "native-activity", __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))
#define LOGF(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__));abort()
#define LOGA(...) ((void)__android_log_print(ANDROID_LOG_ASSERT, "native-activity", __VA_ARGS__))
#endif
