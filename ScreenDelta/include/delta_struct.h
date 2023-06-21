#ifndef __DELTA__STRUCT__
#define __DELTA__STRUCT__

#include <Common.h>

#include <rect.h>

typedef struct {
	Rect rect;
	int compressed_size;
} DeltaPacketInfo;

typedef struct {
	DeltaPacketInfo info;
	union
	{
		char buffer[];
		char* buffer_ptr;
	};
}DeltaPacket;

typedef struct {
	Rect rect;
	int buffer_size;
	int offset; 
} BufferRectDesk;

typedef struct {
	Rect full_rect;
	Rect last_drag_pos;

	BufferRectDesk rects[DEFAULT_BUFFER_SIZE];
	int num_of_rects;

	char buffer[];
}DeltaStruct;

DeltaStruct* create_delta_struct(
	int size, 
	int target_bit_count);

#endif
