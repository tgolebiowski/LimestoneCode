struct GameMemory {
    MeshGeometryData meshData;
    MeshGPUBinding meshBinding;
    ShaderProgramBinding shader;
    ShaderProgramParams params;
    Mat4 m;
    Armature arm;
};

void GameInit( MemorySlab* gameMemory ) {
    if( InitRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) == false ) {
        printf("Failed to init renderer\n");
        return;
    }

    float screenAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    SetOrthoProjectionMatrix( 10.0f, 10.0f * screenAspectRatio, -10.0f, 10.0f );

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;
    LoadMeshDataFromDisk( "Data/ComplexSkeletonDebug.dae", &gMem->meshData, &gMem->arm );
    CreateRenderBinding( &gMem->meshBinding, &gMem->meshData );
    CreateShaderProgram( &gMem->shader, "Data/Shaders/Basic.vert", "Data/Shaders/Basic.frag" );
    SetToIdentity( &gMem->m );
    gMem->params.modelMatrix = &gMem->m;
    gMem->params.sampler1 = 0;
    gMem->params.sampler2 = 0;
    gMem->params.armature = &gMem->arm;

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
    RenderBoundData( &gMem->meshBinding, &gMem->shader, gMem->params );

    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //RenderFramebuffer( &myFramebuffer );
}