#include <string>
#include <assert.h>
#include <cstring>
#include <stdio.h>

#define NULL 0

#include <time.h>
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
typedef tm TimeStruct;

#define global_variable static

#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

#define SIZEOF_GLOBAL_HEAP MEGABYTES( 66 )

struct Stack {
	void* start;
	void* current;
	uint64 size;
};

#define SPACE_IN_STACK(s) s->size - ((intptr)s->current - (intptr)s->start)

Stack AllocateNewStackFromStack( Stack* stack, uint64 newStackSize ) {
	Stack stackSub = { 0, 0, 0 };
	if( SPACE_IN_STACK(stack) > newStackSize ) {
		stackSub.start = stack->current;
		stackSub.current = stack->current;
		stackSub.size = newStackSize;
		stack->current = (void*)( (char*)stack->current + newStackSize );
	} else {
		int64 spaceNeeded = newStackSize - SPACE_IN_STACK(stack);
		assert( spaceNeeded == 0 );
	}
	return stackSub;
}
 
void* StackAlloc( Stack* stack, uint64 sizeInBytes ) {
	if( SPACE_IN_STACK(stack) > sizeInBytes ) {
		void* returnValue = stack->current;
		stack->current = (void*)( (char*)stack->current + sizeInBytes );
		return returnValue;
	} else {
		int64 spaceNeeded = sizeInBytes - SPACE_IN_STACK(stack);
		assert( spaceNeeded == 0 );
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

struct Pool {
	uint32 blockSize;
	uint32 totalSize;
	void* start;
	uint32 inFreeList;
	uint32 freeListHead;
	uint32 initialized;
};

#define BLOCKS_IN_POOL( p ) ( p.totalSize / p.blockSize )

#define ADDR_VIA_INDEX( index, block, start ) (void*)( (intptr)start + (intptr)( block * index ) )

#define INDEX_VIA_ADDR( addr, block, start ) (uint32)( ( ((intptr)addr) - ((intptr)start) ) / block )

Pool InitPool( Stack* baseMem, uint32 blockSize, uint32 numBlocks ) {
	//So a free list index can fit in block
	assert( blockSize > 4 );

	Pool pool = { };
	pool.blockSize = blockSize;
	pool.totalSize = blockSize * numBlocks;
	pool.start = StackAllocAligned( baseMem, pool.totalSize );
	pool.inFreeList = 0;
	pool.freeListHead = 0;
	pool.initialized = 0;

	return pool;
}

void* PoolAlloc( Pool* pool ) {
	//Take from freelist first, since this defrags memory naturally
	if( pool->inFreeList > 0 ) {
		//Get Address to return
		int32 newAllocIndex = pool->freeListHead;
		void* freeAddr = ADDR_VIA_INDEX( newAllocIndex, pool->blockSize, pool->start );

		//Read next block in freelist and store (can be garbage, this won't matter) 
		pool->freeListHead = *((uint32*)freeAddr);
		--pool->inFreeList;

		memset( freeAddr, 0, pool->blockSize );
		return freeAddr;
	}

	//If nothing free list, check if space remains
	bool hasRemainingIndicies = ( pool->totalSize / pool->blockSize ) > pool->initialized;

	//If space remains...
	if( hasRemainingIndicies ) {
		//Alloc as though it were a stack
		void* newAddr = ADDR_VIA_INDEX( pool->initialized, pool->blockSize, pool->start );
		++pool->initialized;

		memset( newAddr, 0, pool->blockSize );
		return newAddr;
	}

	return NULL;
}

void PoolFree( Pool* pool, void* addr ) {
	uint32 indexToFree = INDEX_VIA_ADDR( addr, pool->blockSize, pool->start );

	if( pool->inFreeList > 0 ) {
		*((uint32*)addr) = pool->freeListHead;
	}

	++pool->inFreeList;
	pool->freeListHead = indexToFree;
}

struct InputState {
	enum SPC_KEY_ENUM {
		CTRL = 0, 
		BACKSPACE = 1, 
		TAB = 2, 
		DEL = 3,
		SPACE = 4
	};

	enum MOUSE_BUTTONS {
		LEFT = 0, 
		MIDDLE = 1, 
		RIGHT = 2
	};

	InputState* prevState;

	bool romanCharKeys[32];
	bool spcKeys[8];

	char keysPressedSinceLastUpdate [24];

	//Mouse coords range from -1 to 1, from the top left corner to the bottom right
	float mouseX;
	float mouseY;

	bool mouseButtons[4];

	float leftStick_x, leftStick_y;
	float rightStick_x, rightStick_y;
	float leftTrigger, rightTrigger;
	bool leftBumper, rightBumper;
	bool button1, button2, button3, button4;
	bool specialButtonLeft, specialButtonRight;
};

#ifdef DLL_ONLY
static bool IsKeyDown( InputState* i, char key ) {
	if( key >= 'A' && key <= 'Z' ) key += ( 'a' - 'A' );
	int index = key - 'a';
	return i->romanCharKeys[ index ];
}

static bool IsKeyDown( InputState* i, InputState::SPC_KEY_ENUM key ) {
	return i->spcKeys[ (int)key ];
}

static bool WasKeyPressed( InputState* i, char key ) {
	return !IsKeyDown( i->prevState, key ) && IsKeyDown( i, key );
}

static bool WasKeyPressed( InputState* i, InputState::SPC_KEY_ENUM key ) {
	return !IsKeyDown( i->prevState, key ) && IsKeyDown( i, key );
}

static bool  WasMouseButtonClicked( InputState* i, InputState::MOUSE_BUTTONS b ) {
	return i->mouseButtons[ b ] && !i->prevState->mouseButtons[ b ];
}
 
#endif

struct System {
	uint16 windowWidth;
	uint16 windowHeight;

	void* (*ReadWholeFile) (char* fileName, Stack* tempStorage, int* bytesRead );
	int (*GetMostRecentMatchingFile) (char*, char*);
	int (*TrackFileUpdates)( char* filePath );
	bool (*DidFileUpdate)( int trackingIndex );
	void (*WriteFile)(char* fileName, void* data, int bytesToWrite );
	bool (*HasFocus)(void);
};

/* ----
	Shared Includes
-----*/

#include "Math3D.h"
#include "Renderer.h"
#include "DearImGui_Limestone.h"
#include "Sound.h"
#include "Parsing.h"
#include "Thread.h"

/*--------
	Exported functions in DLL
---------*/

#define GAME_INIT(name) void* name( Stack* mainSlab, RenderDriver* renderDriver, SoundDriver* soundDriver, System* system )
typedef GAME_INIT( gameInit );

#define GAME_UPDATEANDRENDER(name) bool name( void* gameMemory, float millisecondsElapsed, InputState* i, SoundDriver* soundDriver, RenderDriver* renderDriver, System* system, void* imguistate )
typedef GAME_UPDATEANDRENDER( updateAndRender );