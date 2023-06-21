#ifdef _WIN32

#include <Windows.h>
#include <stdio.h>

#include "console.h"

void create_debug_console() {
#ifdef _DEBUG
	AllocConsole();

	FILE* fpFile;
	freopen_s(&fpFile, "CONOUT$", "w", stdout); // redirect stdout to console
	freopen_s(&fpFile, "CONOUT$", "w", stderr); // redirect stderr to console
	freopen_s(&fpFile, "CONIN$", "r", stdin);

#endif
}

#endif