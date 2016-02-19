#ifndef APP_H
#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

// Structs needed by the game the OS needs to "fill out"
struct MemorySlab {
	void* slabStart;
	uint64_t slabSize;
};

/* -------------------------------
	STUFF THE OS PROVIDES THE GAME
----------------------------------*/
int16_t ReadShaderSrcFileFromDisk(const char* fileName, GLchar* buffer, uint16_t bufferLen);

/* --------------------------------
	STUFF THE GAME PROVIDES THE OS
 ----------------------------------*/
bool Update();
void Render();
void GameInit( MemorySlab slab );

#define APP_H
#endif