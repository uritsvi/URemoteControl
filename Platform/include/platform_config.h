#ifndef __CONFIG__
#define __CONFIG__

#ifdef _WIN32
	#include <Windows.h>

	typedef struct {
		HINSTANCE hInstance;
		int cmdShow;
	} PlatformConfig;
#endif

	void init_platform_config(PlatformConfig* config);

	PlatformConfig* get_platform_config();

#endif