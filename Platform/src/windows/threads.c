#ifdef _WIN32

#include <Windows.h>
#include <stdio.h>
#include <synchapi.h>
#include <platfrom_memory_utils.h>
#include <Common.h>

#include "threads.h"


typedef struct {
	HANDLE handle;
	Thread out;

} WIN32Thread;

typedef struct {
	HANDLE handle;
	Mutex out;

} WIN32Mutex;

typedef struct {
	HANDLE handle;
	Event out;

} WIN32Event;

static WIN32Thread* g_Threads[MAX_OBJECTS] = {0};
static WIN32Mutex* g_Mutexes[MAX_OBJECTS] = {0};
static WIN32Event* g_Events[MAX_OBJECTS] = {0};

static int g_NextThreadIndex = 0;
static int g_NextMutexIndex = 0;
static int g_NextCondIndex = 0;
static int g_NextEventIndex = 0;


bool create_thread(thread_function function, 
				   void* parameters, 
				   Thread* out) {

	if (g_NextThreadIndex >= MAX_OBJECTS) {
		return false;
	}
	
	WIN32Thread* _thread = (WIN32Thread*)safe_malloc(sizeof(WIN32Thread));

	HANDLE thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)function, parameters, THREAD_TERMINATE, NULL);
	_thread->handle = thread_handle;

	*out = g_NextThreadIndex++;
	_thread->out = out;

	g_Threads[*out] = _thread;

	return true;
}

bool create_mutex(Mutex* out) {
	if (g_NextMutexIndex >= MAX_OBJECTS) {
		return false;
	}

	HANDLE mutext_handle = CreateMutexA(NULL, FALSE, NULL);
	if (mutext_handle == NULL) {
		return false;
	}

	
	WIN32Mutex* _mutex = (WIN32Mutex*)safe_malloc(sizeof(WIN32Mutex));
	_mutex->handle = mutext_handle;

	*out = g_NextMutexIndex++;
	_mutex->out = *out;

	g_Mutexes[*out] = _mutex;

	return true;
	

}

void lock_mutex(Mutex mutex) {	
	WIN32Mutex* _mutex = g_Mutexes[mutex];
	WaitForSingleObject(_mutex->handle, INFINITE);

}

bool try_lock(Mutex mutex) {
	WIN32Mutex* _mutex = g_Mutexes[mutex];
	DWORD res = WaitForSingleObject(_mutex->handle, 0);
	if (!res) {
		return true;
	}
	return false;
}

void release_mutex(Mutex mutex) {
	
	WIN32Mutex* _mutex = g_Mutexes[mutex];
	ReleaseMutex(_mutex->handle);
}


void clean_up_threads() {
	for (int i = 0; i < MAX_OBJECTS; i++) {
		WIN32Thread* cur_thread = g_Threads[i];
		if (cur_thread == NULL) {
			continue;
		}

		TerminateThread(cur_thread->handle, 0);
	}


}


bool create_event(Event* out) {
	WIN32Event* _event = 
		(WIN32Event*)safe_malloc(sizeof(WIN32Event));

	HANDLE handle = CreateEventA(NULL, TRUE, FALSE, NULL);

	if (_event == NULL) {
		return false;
	}

	_event->handle = handle;

	*out = g_NextEventIndex++;

	_event->out = out;

	g_Events[*out] = _event;

	return true;
}

void wait_event(Event _event) {
	WIN32Event* internal_event = g_Events[_event];

	BOOL res =
		WaitForSingleObject(internal_event->handle, INFINITE);
}

void set_event(Event _event) {
	WIN32Event* internal_event = 
		g_Events[_event];

	
	SetEvent(internal_event->handle);
}

void set_event_and_wait(Event set_on, Event wait_on) {
	WIN32Event* _sing_on = g_Events[set_on];
	WIN32Event* _wait_on = g_Events[wait_on];

	SignalObjectAndWait(_sing_on->handle, _wait_on->handle, INFINITE, FALSE);
}

void reset_event(Event mutex) {
	WIN32Event* _cross_thread_mutex =
		g_Events[mutex];

	BOOL res = ResetEvent(_cross_thread_mutex->handle);
}

void wait_multiple_events(int nun_of_mutexes, Event* events) {
	HANDLE* handles = 
		safe_malloc(nun_of_mutexes * sizeof(HANDLE));

	for (int i = 0; i < nun_of_mutexes; i++) {
		handles[i] = g_Events[events[i]]->handle;
	}

	WaitForMultipleObjects(nun_of_mutexes, handles, TRUE, INFINITE);

	free(handles);
}

#endif