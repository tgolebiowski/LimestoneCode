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
    LoadedSound testSound;
    LoadedSound sound2;
    SlabSubsection_Stack lasResidentStorage;
};
static Mat4 i;
static Mat4 r;


void GameInit( MemorySlab* mainSlab, void* gameMemory, RendererStorage* rendererStoragePtr ) {
    GameMemory* gMem = (GameMemory*)gameMemory;

    gMem->lasResidentStorage = CarveNewSubsection( mainSlab, KILOBYTES( 128 ) );

    float screenAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    SetRendererCameraProjection( 10.0f, 10.0f * screenAspectRatio, -10.0f, 10.0f, &rendererStoragePtr->baseProjectionMatrix );
    SetRendererCameraTransform( rendererStoragePtr, { 0.0f, 1.0f, -2.0f }, { 0.0f, 0.0f, 0.0f } );

    LoadMeshDataFromDisk( "Data/SkeletonDebug.dae", &gMem->lasResidentStorage, &gMem->meshData, &gMem->arm );
    LoadAnimationDataFromCollada( "Data/SkeletonDebug.dae", &gMem->pose, &gMem->arm );
    LoadAnimationDataFromCollada( "Data/SkeletonDebugPose_2.dae", &gMem->pose2, &gMem->arm );
    LoadTextureDataFromDisk( "Data/Textures/green_texture.png", &gMem->texData );

    gMem->testSound = LoadWaveFile( "Data/Sounds/Dawn.wav" );
    gMem->sound2 = LoadWaveFile( "Data/Sounds/SkullKid_Step1_Long.wav" );

    CreateRenderBinding( &gMem->meshData, &gMem->meshBinding );
    CreateShaderProgram( "Data/Shaders/Basic.vert", "Data/Shaders/Basic.frag", &gMem->shader );
    CreateTextureBinding( &gMem->texData, &gMem->texBinding );
    gMem->params = CreateShaderParamSet( &gMem->shader );
    SetToIdentity( &gMem->m );
    SetUniform( &gMem->params, "modelMatrix", (void*)&gMem->m );
    SetUniform( &gMem->params, "tex1", (void*)&gMem->texBinding );
    SetUniform( &gMem->params, "skeleton", (void*)&gMem->arm.boneTransforms[0] );

    SetToIdentity( &i ); SetToIdentity( &r );
    SetTranslation( &i, 0.0f, -2.5f, 0.0f );
    SetScale( &i, 0.5f, 0.5f, 0.5f );
    SetRotation( &r, 0.0f, 1.0f, 0.0f, PI / 128.0f );

    return;
}

bool Update( void* gameMemory, float millisecondsElapsed, SoundRenderBuffer* sound, PlayingSound* activeSoundList ) {
    GameMemory* gMem = (GameMemory*)gameMemory;

    if( IsKeyDown( 'r' ) )
        i = MultMatrix( i, r );

    float weight;
    ArmatureKeyFrame* keys[2];
    keys[0] = &gMem->pose;
    keys[1] = &gMem->pose2;

    const float rate = ( 2.0f * PI ) / 1000.0f;
    static float current = 0.0f;
    if( IsKeyDown( 'd' ) ) {
        current += ( rate * millisecondsElapsed );
    }

    static bool wasPDown = false;
    bool pDown = IsKeyDown( 'p' );

    weight = ( cosf( current ) + 1.0f ) / 2.0f;

    if( IsKeyDown( 'z' ) ) {
        if( IsKeyDown( '1' ) ) {
            weight = 1.0f;
        }
        if( IsKeyDown( '2' ) ) {
            weight = 0.0f;
        }
        ArmatureKeyFrame blended = BlendKeyFrames( &gMem->pose, &gMem->pose2, weight, gMem->arm.boneCount );
        ApplyKeyFrameToArmature( &blended, &gMem->arm );
    } else {
        if( IsKeyDown( '1' ) ) {
            ApplyKeyFrameToArmature( keys[0], &gMem->arm );
        } else {
            ApplyKeyFrameToArmature( keys[1], &gMem->arm);
        }
    }

    static bool hKeyLastState = false;
    bool hKeyThisState = IsKeyDown( 'h' );
    if( !hKeyLastState && hKeyThisState ) {
        QueueLoadedSound( &gMem->sound2, activeSoundList );
    }
    hKeyLastState = hKeyThisState;

    static bool kKeyLastState = false;
    bool kKeyThisState = IsKeyDown( 'k' );
    if( !kKeyLastState && kKeyThisState ) {
        QueueLoadedSound( &gMem->testSound, activeSoundList );
    }
    kKeyLastState = kKeyThisState;

    return true;
}

void Render( void* gameMemory, RendererStorage* rendererStorage ) {
    GameMemory* gMem = (GameMemory*)gameMemory;

    *gMem->params.modelMatrix = i;
    RenderBoundData( rendererStorage, &gMem->meshBinding, &gMem->shader, gMem->params );
}