#define DLL_ONLY
#include "App.h"

#include "DearImGui_Limestone.h"

struct GameMemory {
	Stack imguiStack;
	void* imguiState;
};

extern "C" GAME_INIT( GameInit ) {
    Stack lostSection = AllocateNewStackFromStack( mainSlab, KILOBYTES( 1 ) );
    void* gMemPtr = StackAllocAligned( &lostSection, sizeof( GameMemory ) );
    GameMemory* gMem = (GameMemory*)gMemPtr;

    gMem->imguiStack = AllocateNewStackFromStack( mainSlab, MEGABYTES( 8 ) );
    gMem->imguiState = InitImGui_LimeStone( 
    	&gMem->imguiStack, 
    	renderDriver,
    	system->windowWidth,
    	system->windowHeight
    );

    return gMemPtr;
}

extern "C" GAME_UPDATEANDRENDER( UpdateAndRender ) {
	GameMemory* gMem = (GameMemory*)gameMemory;

	UpdateImgui( i, gMem->imguiState, system->windowWidth, system->windowHeight );
    
	ImGui::ShowTestWindow();

	ImGui::Render();

    return true;
}