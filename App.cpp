struct GameMemory {
    MeshGeometryData meshData;
    MeshGPUBinding meshBinding;
    ShaderProgramBinding shader;
    ShaderProgramParams params;
    TextureData texData;
    TextureBindingID texBinding;
    Mat4 m;
    Armature arm;
    ArmatureKeyFrame pose;
    ArmatureKeyFrame pose2;
};
static Mat4 i;
static Mat4 r;


void GameInit( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    if( InitRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) == false ) {
        printf("Failed to init renderer\n");
        return;
    }

    float screenAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    SetRendererCameraProjection( 10.0f, 10.0f * screenAspectRatio, -10.0f, 10.0f );
    SetRendererCameraTransform( { 0.0f, 1.0f, -2.0f }, { 0.0f, 0.0f, 0.0f } );

    LoadMeshDataFromDisk( "Data/SkeletonDebug.dae", &gMem->meshData, &gMem->arm );
    LoadAnimationDataFromCollada( "Data/SkeletonDebug.dae", &gMem->pose, &gMem->arm );
    LoadAnimationDataFromCollada( "Data/SkeletonDebugPose_2.dae", &gMem->pose2, &gMem->arm );
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
    SetTranslation( &i, 0.0f, -2.5f, 0.0f );
    SetScale( &i, 0.5f, 0.5f, 0.5f );
    SetRotation( &r, 0.0f, 1.0f, 0.0f, PI / 128.0f );

    return;
}

bool Update( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    if( IsKeyDown( 'r' ) )
        i = MultMatrix( i, r );

    float weights[2];
    ArmatureKeyFrame* keys[2];
    keys[0] = &gMem->pose;
    keys[1] = &gMem->pose2;

    const float tick = PI / 120;
    static float current = 0.0f;
    if( IsKeyDown( 'd' ) ) {
        current += tick;
    }

    static bool wasPDown = false;
    bool pDown = IsKeyDown( 'p' );

    weights[0] = ( cosf( current ) + 1.0f ) / 2.0f;
    weights[1] = ( 1.0f - weights[0] );

    if( IsKeyDown( 'z' ) ) {
        if( IsKeyDown( '1' ) ) {
            weights[0] = 1.0f; weights[1] = 0.0f;
        }
        if( IsKeyDown( '2' ) ) {
            weights[0] = 0.0f; weights[1] = 1.0f;
        }
        ApplyBlendedArmatureKeyFrames( 2, keys, (float*)weights, &gMem->arm, ( !wasPDown && pDown ) );
    } else {
        if( IsKeyDown( '1' ) ) {
            ApplyArmatureKeyFrame( keys[0], &gMem->arm, ( !wasPDown && pDown ) );
        } else {
            ApplyArmatureKeyFrame( keys[1], &gMem->arm, ( !wasPDown && pDown ) );
        }
    }

    wasPDown = pDown;

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

    //RenderTexturedQuad( &gMem->texBinding, 0.5f, 0.5f, -0.5f, 0.5f );

    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //RenderFramebuffer( &myFramebuffer );
}