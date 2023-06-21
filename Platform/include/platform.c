#ifdef _WIN32

#include <Windows.h>

#include <Common.h>

#include "platform.h"

void get_runing_dir(
	char* buffer) {

	GetModuleFileNameA(NULL, 
					   buffer, 
					   DEFAULT_BUFFER_SIZE);
	
	char* last_slash = strrchr(buffer, '\\');
	if (last_slash != NULL) {
		*last_slash = '\0';
	}

}

#endif