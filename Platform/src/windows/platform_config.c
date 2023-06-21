#ifdef _WIN32

#include "platform_config.h"

static PlatformConfig* g_Config;

void init_platform_config(PlatformConfig* config) {
	g_Config = config;
}

PlatformConfig* get_platform_config() {
	return g_Config;
}

#endif