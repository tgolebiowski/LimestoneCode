#define DLL_ONLY
#include "App.h"

struct GameMemory {

};

extern "C" GAME_INIT( GameInit ) {
    Stack lostSection = AllocateNewStackFromStack( mainSlab, KILOBYTES( 1 ) );
    void* gMemPtr = StackAllocAligned( &lostSection, sizeof( GameMemory ) );
    GameMemory* gMem = (GameMemory*)gMemPtr;

    return gMemPtr;
}

extern "C" GAME_UPDATEANDRENDER( UpdateAndRender ) {
	GameMemory* gMem = (GameMemory*)gameMemory;

    
    return true;
}