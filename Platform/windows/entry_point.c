#ifdef _WIN32

#include <Windows.h>

#include <platfrom_memory_utils.h>

#include "entry_point.h"
#include "platform_config.h"

int WINAPI WinMain(HINSTANCE hInstance, 
				   HINSTANCE pInstance, 
				   LPSTR cmd, 
				   int cmdShow) {
	PlatformConfig* config = safe_malloc(sizeof(PlatformConfig));

	config->hInstance = hInstance;
	config->cmdShow = cmdShow;
	
	init_platform_config(config);
	entry_point(__argc, __argv);
}

#endif