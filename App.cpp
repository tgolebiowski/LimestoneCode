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

    gMem->testSound = LoadWaveFile( "Data/Sounds/Dawn.wav" );
    gMem->sound2 = LoadWaveFile( "Data/Sounds/SkullKid_Step1_Long.wav" );

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

bool Update( MemorySlab* gameMemory, float millisecondsElapsed, SoundRenderBuffer* sound, PlayingSound* activeSoundList ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

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

    ArmatureKeyFrame testPose = gMem->pose;
    ArmatureKeyFrame blendedPose = BlendKeyFrames( &gMem->pose, &gMem->pose2, 1.0f, gMem->arm.boneCount );

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

void OutputAudio( MemorySlab* gameMemory ) {
    
}

void Render( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    *gMem->params.modelMatrix = i;
    RenderBoundData( &gMem->meshBinding, &gMem->shader, gMem->params );
}