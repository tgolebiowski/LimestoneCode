#ifndef APP_H
#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

#include <stdint.h>
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;

#include "Math3D.h"
#include "Renderer.h"

uint16 SCREEN_WIDTH = 640;
uint16 SCREEN_HEIGHT = 480;

struct MemorySlab {
	void* slabStart;
	uint64 slabSize;
};

/* --------------------------------------------------------------------------
	                      STUFF THE OS PROVIDES THE GAME
-----------------------------------------------------------------------------*/
///Set inputted values to Normalized Window Coordinates (0,0 is center, ranges go from -1 to +1)
void GetMousePosition( float* x, float* y );

/* --------------------------------------------------------------------------
	                      STUFF THE GAME PROVIDES THE OS
 ----------------------------------------------------------------------------*/
bool Update( MemorySlab* gameMemory );
void Render( MemorySlab* gameMemory );
void GameInit( MemorySlab* gameMemory );

#define APP_H
#endif