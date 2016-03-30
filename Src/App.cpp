struct GameMemory {
    MeshGeometryData meshData;
    MeshGPUBinding meshBinding;
    ShaderProgramBinding shader;
    ShaderProgramParams params;
};

void GameInit( MemorySlab* gameMemory ) {
    if( InitRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) == false ) {
        printf("Failed to init renderer\n");
        return;
    }

    float screenAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    SetOrthoProjectionMatrix( 10.0f, 10.0f * screenAspectRatio, -1.0f, 1.0f );

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;
    LoadMeshDataFromDisk( "Data/ComplexSkeletonDebug.dae", &gMem->meshData );

    return;
}

bool Update( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

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

    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //RenderFramebuffer( &myFramebuffer );
}