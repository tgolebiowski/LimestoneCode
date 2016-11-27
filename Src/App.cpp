#define DLL_ONLY
#include "App.h"
#include "DebugDraw.h"
#include "DearImGui_Limestone.h"

#include <cstdlib>

struct GameMemory {

};

extern "C" GAME_INIT( GameInit ) {

}

extern "C" GAME_UPDATEANDRENDER( UpdateAndRender ) {

    return true;
} 