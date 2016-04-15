struct GameMemory {
    MeshGeometryData meshData;
    MeshGPUBinding meshBinding;
    ShaderProgramBinding shader;
    ShaderProgramParams params;
    TextureData texData;
    TextureBindingID texBinding;
    Mat4 m;
    Armature arm;
    ArmaturePose pose;
};
static Mat4 i;
static Mat4 r;

void GameInit( MemorySlab* gameMemory ) {
    if( InitRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) == false ) {
        printf("Failed to init renderer\n");
        return;
    }

    float screenAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    SetOrthoProjectionMatrix( 10.0f, 10.0f * screenAspectRatio, -10.0f, 10.0f );

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;
    LoadMeshDataFromDisk( "Data/ComplexSkeletonDebug.dae", &gMem->meshData, &gMem->arm );
    LoadAnimationDataFromCollada( "Data/ComplexSkeletonDebug.dae", &gMem->pose, &gMem->arm );
    LoadTextureDataFromDisk( "Data/Textures/green_texture.png", &gMem->texData );
    CreateRenderBinding( &gMem->meshData, &gMem->meshBinding );
    CreateShaderProgram( "Data/Shaders/Basic.vert", "Data/Shaders/Basic.frag", &gMem->shader );
    CreateTextureBinding( &gMem->texData, &gMem->texBinding );
    SetToIdentity( &gMem->m );
    gMem->params.modelMatrix = &gMem->m;
    gMem->params.sampler1 = gMem->texBinding;
    gMem->params.sampler2 = 0;
    gMem->params.armature = &gMem->arm;

    SetToIdentity( &i ); SetToIdentity( &r );
    SetTranslation( &i, 0.0f, -3.0, 0.0f );
    SetScale( &i, 0.5f, 0.5f, 0.5f );
    SetRotation( &r, 0.0f, 1.0f, 0.0f, PI / 128.0f );
    
    ApplyArmaturePose( &gMem->arm, &gMem->pose );

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

    i = MultMatrix( i, r );

    return true;
}

void Render( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    //glClear( GL_DEPTH_BUFFER_BIT );
    //glBindFramebuffer( GL_FRAMEBUFFER, myFramebuffer.framebufferPtr );
    //glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, myFramebuffer.framebufferTexture.textureID, 0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    *gMem->params.modelMatrix = i;
    RenderBoundData( &gMem->meshBinding, &gMem->shader, gMem->params );
    RenderArmatureAsLines( &gMem->arm, i, { 1.0f, 0.0f, 0.0f } );

    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //RenderFramebuffer( &myFramebuffer );
}