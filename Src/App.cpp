#define APP_H
#include "App.h"

struct GameMemory {

};

extern "C" GAME_INIT( GameInit ) {
    Stack lostSection = AllocateStackInGlobalMem( mainSlab, KILOBYTES( 1 ) );
    void* gMemPtr = StackAllocA( &lostSection, sizeof( GameMemory ) );
    GameMemory* gMem = (GameMemory*)gMemPtr;

    return gMemPtr;
}

extern "C" GAME_UPDATEANDRENDER( UpdateAndRender ) {

    return true;
}