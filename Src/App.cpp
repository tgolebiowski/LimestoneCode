#include "AI.h"

struct GameMemory {
    AIDataStorage aiData;
};

float x, y;

void GameInit( MemorySlab* gameMemory ) {
    if( InitRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) == false ) {
        printf("Failed to init renderer\n");
        return;
    }

    float screenAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    SetOrthoProjectionMatrix( 10.0f, 10.0f, -10.0f, 10.0f );

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    InitAIComponents( &gMem->aiData );

    return;
}

bool Update( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    GetMousePosition( &x, &y );
    x *= 5.0f; y *= 5.0f;
    UpdateAI( &gMem->aiData, { x, y, 0.0f } );

    return true;
}

void Render( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    //glClear( GL_DEPTH_BUFFER_BIT );
    //glBindFramebuffer( GL_FRAMEBUFFER, myFramebuffer.framebufferPtr );
    //glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, myFramebuffer.framebufferTexture.textureID, 0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    DebugRenderAI( &gMem->aiData );
    RenderDebugCircle( { x, y, 0.0f }, 0.05f );

    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //RenderFramebuffer( &myFramebuffer );
}