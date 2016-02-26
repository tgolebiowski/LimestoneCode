#ifndef APP_H
#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

//TODO: Remove STL depedencies, also, putting the map include after the Math3D include breaks memsets in Math3D
#include <map>
#include <set>
#include <utility>
#include "Math3D.cpp"
#include "ClayRenderer.h"

// Structs needed by the game the OS needs to "fill out"
struct MemorySlab {
	void* slabStart;
	uint64_t slabSize;
};

/* -------------------------------
	STUFF THE OS PROVIDES THE GAME
----------------------------------*/
///Return 0 on success, required buffer length if buffer is too small, or -1 on other OS failure
int16_t ReadShaderSrcFileFromDisk(const char* fileName, GLchar* buffer, uint16_t bufferLen);

/* --------------------------------
	STUFF THE GAME PROVIDES THE OS
 ----------------------------------*/
bool Update();
void Render();
void GameInit( MemorySlab slab );

#define APP_H
#endif