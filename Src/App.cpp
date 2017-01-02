#define DLL_ONLY
#include "App.h"


struct GameMemory {

};

extern "C" GAME_INIT( GameInit ) {
	return NULL;
}

extern "C" GAME_UPDATEANDRENDER( UpdateAndRender ) {
	ImGui::SetInternalState( imguistate );

    return true;
} 