#pragma once

#include "platform_detection.h"

#define LWMAX(a, b)					((a) > (b) ? (a) : (b))
#define LWMIN(a, b)					((a) < (b) ? (a) : (b))
#define LWCLAMP(v, a, b)			((v) < (a) ? (a) : (v) > (b) ? (b) : (v))
#define LWDEG2RAD(x)				((x) / 180.0 * M_PI)
#define LWRAD2DEG(x)				((x) / M_PI * 180.0)

#define __WIDEN_TEXT(quote) L##quote
#define WIDEN_TEXT(quote) __WIDEN_TEXT(quote)   // r_winnt

#ifdef SWIG
#   define PATH_SEPARATOR "\\"
#   define ASSETS_BASE_PATH "assets" PATH_SEPARATOR
#   define LwStaticAssert(x,y)
#   define ARRAY_SIZE(arr) 0 // Swig has no support of compile time arithmetic
#elif LW_PLATFORM_WIN32
#   include <stdlib.h>
#   define PATH_SEPARATOR "\\"
#   define ASSETS_BASE_PATH "assets" PATH_SEPARATOR
#   define LwStaticAssert(x,y) _STATIC_ASSERT(x)
#   define ARRAY_SIZE _countof
#elif LW_PLATFORM_OSX
#   define PATH_SEPARATOR "/"
#   define ASSETS_BASE_PATH "/Users/kimgeoyeob/laidoff/assets/"
#   define LwStaticAssert(x,y) //_STATIC_ASSERT(x) // diabled...
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#elif LW_PLATFORM_LINUX
#   define PATH_SEPARATOR "/"
#   define ASSETS_BASE_PATH "/home/gasbank/laidoff/assets/"
#   define LwStaticAssert(x,y) //_STATIC_ASSERT(x) // diabled...
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#elif LW_PLATFORM_ANDROID || LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
#   define PATH_SEPARATOR "/"
#   define ASSETS_BASE_PATH

#   define LwStaticAssert(x,y) //static_assert((x),(y))
#   define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#   define __must_be_array(a) \
 BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))

//#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#elif LW_PLATFORM_RPI
#   define PATH_SEPARATOR "/"
#   define ASSETS_BASE_PATH "assets" PATH_SEPARATOR
#   define LwStaticAssert(x,y)
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef M_PI
#   define M_PI       (3.14159265358979323846)  /* pi */
#endif

#if LW_PLATFORM_OSX || LW_PLATFORM_RPI || LW_PLATFORM_LINUX
#   define HRESULT int
#elif LW_PLATFORM_ANDROID || LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
#   define HRESULT int
#endif

#define ARRAY_ITERATE_VALID(t, v) \
    for (int i = 0; i < ARRAY_SIZE((v)); i++) \
    { \
        if (v[i].valid) { \
			t* e = &v[i];

#define ARRAY_ITERATE_VALID_END() \
		} \
    }

#define ARRAY_ITERATE_PTR_VALID(t, v) \
    for (int i = 0; i < ARRAY_SIZE((v)); i++) \
    { \
        if (v[i]->valid) { \
			t* e = v[i];

#define ARRAY_ITERATE_VALID_END() \
		} \
    }

#define LWEPSILON (1e-3)

#define LW_STRINGIFY_(x) #x
#define LW_STRINGIFY(x) LW_STRINGIFY_(x)
#define __LW_STRING_LINE__ LW_STRINGIFY(__LINE__)
#define LWLOGPOS __FILE__ "(" __LW_STRING_LINE__ "): "

#define LWOFFSETOF(s,m) ((size_t)&(((s*)0)->m))

#if LW_PLATFORM_WIN32
typedef int socklen_t;
#endif

#define LW_GL_VIEWPORT() glViewport(0, 0, pLwc->width, pLwc->height)