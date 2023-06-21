#ifndef __THREADS__
#define __THREADS__


#include <stdbool.h>

typedef void(*thread_function)(void* parameters);

typedef int Thread;

typedef int Mutex;
typedef int Event;

bool create_thread(thread_function function, 
							  void* parameters, 
							  Thread* out);
bool create_mutex(Mutex* out);
bool create_event(Event* out);
 
void wait_event(Event _event);

void set_event_and_wait(Event set_on, 
						Event wait_on);

void set_event(Event _event);
void reset_event(Event _event);

void wait_multiple_events(int nun_of_mutexes, 
						  Event* events);

void lock_mutex(Mutex mutex);
bool try_lock(Mutex mutex);
void release_mutex(Mutex mutex);

void clean_up_threads();


#endif
