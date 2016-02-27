#ifndef APP_H
#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

#include <stdint.h>
//Idea is from handmade hero
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;

//TODO: Remove STL depedencies, also, putting the map include after the Math3D include breaks memsets in Math3D
#include <map>
#include "Math3D.cpp"
#include "ClayRenderer.h"

struct MemorySlab {
	void* slabStart;
	uint64 slabSize;
};

/* -------------------------------
	STUFF THE OS PROVIDES THE GAME
----------------------------------*/
///Return 0 on success, required buffer length if buffer is too small, or -1 on other OS failure
int16 ReadShaderSrcFileFromDisk(const char* fileName, GLchar* buffer, uint16 bufferLen);

///Set inputted values to Normalized Window Coordinates (0,0 is center, ranges go from -1 to +1)
void GetMousePosition( float* x, float* y );

/* --------------------------------
	STUFF THE GAME PROVIDES THE OS
 ----------------------------------*/
bool Update( MemorySlab* gameMemory );
void Render( MemorySlab* gameMemory );
void GameInit( MemorySlab* gameMemory );

#define APP_H
#endif