#ifdef _WIN32	

#include <Windows.h>
#include <threads.h>

#include "error.h"

static ShutDown g_ShutdownCallback;
static Mutex g_ShutdownLock;

void init_error(ShutDown shutdown) {

	g_ShutdownCallback = shutdown;
	create_mutex(&g_ShutdownLock);
}

void platform_exit_with_error(const char* error_msg) {
	lock_mutex(g_ShutdownLock);

#ifdef _DEBUG
	MessageBoxA(NULL, error_msg, NULL, MB_OK);

#endif

	if (g_ShutdownCallback != NULL) {
		g_ShutdownCallback();
	}

	exit(-1);


	release_mutex(g_ShutdownLock);
}

#endif