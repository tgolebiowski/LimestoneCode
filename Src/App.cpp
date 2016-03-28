#include "AI.h"
struct GameMemory {
    float mouseX, mouseY;
    AIDataStorage aiData;
};

void GameInit( MemorySlab* gameMemory ) {
    if( InitRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) == false ) {
        printf("Failed to init renderer\n");
        return;
    }

    float screenAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    SetOrthoProjectionMatrix( 10.0f, 10.0f * screenAspectRatio, -1.0f, 1.0f );

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;
    InitAIComponents( &gMem->aiData );

    return;
}

bool Update( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    float mouseX, mouseY;
    GetMousePosition( &mouseX, &mouseY );
    mouseX *= 5.0f;
    mouseY *= 5.0f * (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    UpdateAI( &gMem->aiData, { mouseX, mouseY, 0.0f } );
    gMem->mouseX = mouseX;
    gMem->mouseY = mouseY;

    static bool pState = false;
    bool newState = IsKeyDown( 'p' ); 
    if( !pState && newState ) {
        printf("Got a key\n");
    }
    pState = newState;

    return true;
}

void Render( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    //glClear( GL_DEPTH_BUFFER_BIT );
    //glBindFramebuffer( GL_FRAMEBUFFER, myFramebuffer.framebufferPtr );
    //glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, myFramebuffer.framebufferTexture.textureID, 0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    DebugRenderAI( &gMem->aiData );
    RenderDebugCircle( { gMem->mouseX, gMem->mouseY, 0.0f }, 0.25, { 0.05, 0.85, 0.12 } );


    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //RenderFramebuffer( &myFramebuffer );
}