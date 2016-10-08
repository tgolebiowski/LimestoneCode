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
#include "Memory.h"

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

#define GAME_INIT(name) void* name( GlobalMem* mainSlab, RenderDriver* renderDriver, SoundDriver* soundDriver, System* system )
typedef GAME_INIT( gameInit );

#define GAME_UPDATEANDRENDER(name) bool name( void* gameMemory, float millisecondsElapsed, InputState* i, SoundDriver* soundDriver, RenderDriver* renderDriver, System* system )
typedef GAME_UPDATEANDRENDER( updateAndRender );