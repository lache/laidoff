#include "lwtimepoint.h"
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
#include "GLFW/glfw3.h"
#endif

void lwtimepoint_now(LWTIMEPOINT* tp) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	tp->last_time = glfwGetTime();
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	tp->last_time = now;
#endif
}

double lwtimepoint_diff(const LWTIMEPOINT* a, const LWTIMEPOINT* b) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	return a->last_time - b->last_time;
#else
	long nsec_diff = a->last_time.tv_nsec - b->last_time.tv_nsec;
	long sec_diff = a->last_time.tv_sec - b->last_time.tv_sec;

	if (nsec_diff < 0) {
		nsec_diff += 1000000000LL;
		sec_diff--;
	}

	return sec_diff + (double)nsec_diff / 1e9;
#endif
}

long lwtimepoint_get_second_portion(const LWTIMEPOINT* tp) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	return (int)tp->last_time;
#else
	return (long)tp->last_time.tv_sec;
#endif
}

long lwtimepoint_get_nanosecond_portion(const LWTIMEPOINT* tp) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	return (long)((tp->last_time - (int)tp->last_time) * 1e9);
#else
	return (long)tp->last_time.tv_nsec;
#endif
}
