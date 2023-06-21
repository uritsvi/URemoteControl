#include <Common.h>

#include <platfrom_memory_utils.h>
#include <zero_memory.h>

#include "delta_struct.h"


DeltaStruct* create_delta_struct(
	int size, 
	int target_bit_count) {
	
	DeltaStruct* delta_struct = 
		safe_malloc(size);

	ZeroMemory(delta_struct, sizeof(*delta_struct));

	return delta_struct;
}
