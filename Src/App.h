#include <string>
#include <assert.h>
#include <cstring>
#include <stdio.h>
#define NULL 0

#include <stdint.h>
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef uintptr_t uintptr;
typedef intptr_t intptr;

#define global_variable static

/* --------------------------------------------------------------------------
	                      STUFF THE OS PROVIDES THE GAME
-----------------------------------------------------------------------------*/

#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

struct Stack {
	void* start;
	void* current;
	uint64 size;
};

#define SPACE_IN_STACK(s) s->size - ((intptr)s->current - (intptr)s->start)

Stack AllocateNewStackFromStack( Stack* slab, uint64 newStackSize ) {
	Stack slabSub = { 0, 0, 0 };
	if( SPACE_IN_STACK(slab) > newStackSize ) {
		slabSub.start = slab->current;
		slabSub.current = slab->current;
		slabSub.size = newStackSize;
		slab->current = (void*)( (char*)slab->current + newStackSize );
	} else {
		assert(false);
	}
	return slabSub;
}

void* StackAlloc( Stack* stack, uint64 sizeInBytes ) {
	if( SPACE_IN_STACK(stack) > sizeInBytes ) {
		void* returnValue = stack->current;
		stack->current = (void*)( (char*)stack->current + sizeInBytes );
		return returnValue;
	} else {
		assert(false);
		return NULL;
	}
}

//Naive implementation based on examples in "Game Engine Architechture"
void* StackAllocAligned( Stack* stack, uint64 size, uint8 alignment = 8 ) {
	//Add some padding so we have enough room to align
	uint64 expandedSize = size + alignment;

	//Determine how "off" the basic allocated pointer is
	uintptr rawAddress = (uintptr)StackAlloc( stack, expandedSize );
	uintptr alignmentMask = ( alignment - 1 );
	uintptr misalignment = ( rawAddress & alignmentMask );

	//Adjust the pointer to be aligned and return that
	uintptr adjustment = alignment - misalignment;
	uintptr alignedAddress = rawAddress + adjustment;
	return (void*)alignedAddress;
}

void ClearStack( Stack* stack ) {
	stack->current = stack->start;
}

//TODO: This "leaks" memory due to padding, track amount of padding in byte ptr - 1
//And free the other stuff too
void FreeFromStack( Stack* stack, void* ptr ) {
	if( (intptr)ptr > (intptr)stack->current ) {
		stack->current = ptr;
	}
}

struct System {
	uint16 windowWidth;
	uint16 windowHeight;

	void* (*ReadWholeFile) (char*, Stack*);
	int (*GetMostRecentMatchingFile) (char*, char*);
	int (*TrackFileUpdates)( char* filePath );
	bool (*DidFileUpdate)( int trackingIndex );
};

#include "Math3D.h"
#include "Renderer.h"
#include "Sound.h"
#include "Input.h"

#define GAME_INIT(name) void* name( Stack* mainSlab, RenderDriver* renderDriver, SoundDriver* soundDriver, System* system )
typedef GAME_INIT( gameInit );

#define GAME_UPDATEANDRENDER(name) bool name( void* gameMemory, float millisecondsElapsed, InputState* i, SoundDriver* soundDriver, RenderDriver* renderDriver, System* system )
typedef GAME_UPDATEANDRENDER( updateAndRender );