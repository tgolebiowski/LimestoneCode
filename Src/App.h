#ifndef APP_H

#include <stdint.h>
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;

#include "Memory.h"
#include "Math3D.h"
#include "Renderer.h"

/* --------------------------------------------------------------------------
	                      STUFF THE OS PROVIDES THE GAME
-----------------------------------------------------------------------------*/
///Set inputted values to Normalized Window Coordinates (0,0 is center, ranges go from -1 to +1)
void GetMousePosition( float* x, float* y );
///Currently limiting it to Ascii Table
bool IsKeyDown( uint8 keyChar );

bool IsControllerButtonDown( uint8 buttonIndex );
void GetControllerStickState( uint8 stickIndex, float* x, float* y );
float GetTriggerState( uint8 triggerIndex );

/* --------------------------------------------------------------------------
	                      STUFF THE GAME PROVIDES THE OS
 ----------------------------------------------------------------------------*/
bool Update( MemorySlab* gameMemory );
void Render( MemorySlab* gameMemory );
void GameInit( MemorySlab* gameMemory );

#define APP_H
#endif