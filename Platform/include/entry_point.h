#ifndef __ENTRY__POINT__
#define __ENTRY__POINT__

#ifdef _WIN32
	#pragma comment(linker, "/subsystem:windows")
#endif

extern void entry_point(int argc, char* argv[]);

#endif
