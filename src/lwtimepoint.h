#pragma once

#include "platform_detection.h"

#define LW_TIMESPEC_AVAILABLE !(LW_PLATFORM_WIN32 || LW_PLATFORM_OSX)

#if LW_TIMESPEC_AVAILABLE
#include <time.h> // ios struct timespec
#endif

typedef struct _LWTIMEPOINT {
#if LW_TIMESPEC_AVAILABLE
	struct timespec last_time;
#else
	double last_time;
#endif
} LWTIMEPOINT;

void lwtimepoint_now(LWTIMEPOINT* tp);
double lwtimepoint_diff(const LWTIMEPOINT* a, const LWTIMEPOINT* b);
long lwtimepoint_get_second_portion(const LWTIMEPOINT* tp);
long lwtimepoint_get_nanosecond_portion(const LWTIMEPOINT* tp);
