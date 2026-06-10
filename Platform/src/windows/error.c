#ifdef _WIN32

#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <platform_threads.h>

#include "error.h"

static ShutDown g_ShutdownCallback;
static Mutex g_ShutdownLock;

void init_error(ShutDown shutdown) {

	g_ShutdownCallback = shutdown;
	create_mutex(&g_ShutdownLock);
}

static void _log_error_to_file(const char* error_msg) {
	char path[MAX_PATH];
	DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
	if (n == 0 || n >= MAX_PATH) {
		return;
	}

	char* slash = strrchr(path, '\\');
	if (slash != NULL) {
		*(slash + 1) = '\0';
	} else {
		path[0] = '\0';
	}

	char log_path[MAX_PATH];
	_snprintf_s(
		log_path,
		MAX_PATH,
		_TRUNCATE,
		"%sclient_error_%lu.log",
		path,
		(unsigned long)GetCurrentProcessId());

	FILE* f = NULL;
	if (fopen_s(&f, log_path, "a") == 0 && f != NULL) {
		fprintf(f, "%s", error_msg);
		fflush(f);
		fclose(f);
	}
}

void platform_exit_with_error(const char* error_msg) {
	lock_mutex(g_ShutdownLock);

	/* Always record the failure to a file so it is visible even when the
	 * MessageBox can not be shown (e.g. non-interactive launch). */
	_log_error_to_file(error_msg);

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