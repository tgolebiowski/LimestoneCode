static OpenGLFramebuffer myFramebuffer;

struct GameMemory {
    ShaderProgram lineShader;
};

static float angle;
static Mat4 myCameraMatrix;
static Mat4 baseOrthoMat;
static Vec3 cameraPosition;
static MeshData meshData1;

static OpenGLMeshBinding meshBinding1;
static Skeleton skeleton1;
static ArmatureKeyframe frame1;

static ShaderProgram shader1;
static ShaderTextureSet texSet1;
static OpenGLTexture texture1;

bool InitOpenGLRenderer( const float screen_w, const float screen_h ) {
    printf( "Vendor: %s\n", glGetString( GL_VENDOR ) );
    printf( "Renderer: %s\n", glGetString( GL_RENDERER ) );
    printf( "GL Version: %s\n", glGetString( GL_VERSION ) );
    printf( "GLSL Version: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

    GLint k;
    glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &k );
    printf("Max texture units: %d\n", k);

    //Initialize clear color
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glViewport( 0.0f, 0.0f, screen_w, screen_h );

    float screenAspectRatio = screen_w / screen_h;
    InitOrthoCameraMatrix( &baseOrthoMat, 10.0f * screenAspectRatio, 10.0f, 0.1f, 20.0f );
    cameraPosition = {0.0f, 0.0f, 0.0f};

    //Initialize framebuffer
    CreateFrameBuffer( &myFramebuffer );
    CreateShader( &framebufferShader, "Data/Shaders/Framebuffer.vert", "Data/Shaders/Framebuffer.frag" );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR ) {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        return false;
    } else {
        printf( "Initialized OpenGL\n" );
    }

    return true;
}

void GameInit( MemorySlab* gameMemory ) {
    if( InitOpenGLRenderer( 640, 480 ) == false ) {
        printf("Failed to init OpenGL renderer\n");
        return;
    }

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;
    CreateShader( &primitiveShader, "Data/Shaders/Line.vert", "Data/Shaders/Line.frag" );
    CreateShader( &shader1, "Data/Shaders/Vert.vert", "Data/Shaders/Frag.frag" );
    SetToIdentity( &meshData1.modelMatrix );
    SetTranslation( &meshData1.modelMatrix, 0.0f, -1.0f, 0.0f );

    angle = 0.0f;

    LoadMeshViaAssimp( &meshData1, &skeleton1, &frame1, "Data/SkeletonDebug.dae" );
    CreateGLMeshBinding( &meshBinding1, &meshData1 );
    LoadTextureFromFile( &texture1, "Data/Textures/green_texture.png");
    texSet1.count = 1;
    texSet1.associatedShader = &shader1;
    texSet1.shaderSamplerPtrs[0] = shader1.samplerPtrs[0];
    texSet1.shaderSamplerPtrs[1] = shader1.samplerPtrs[1];
    texSet1.texturePtrs[0] = texture1.textureID;
    texSet1.texturePtrs[1] = 0;

    printf("Init went well\n");
    return;
}

bool Update( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    const float angleTick = PI / 128.0f;
    angle += angleTick;
    if( angle > PI * 2.0f ) angle -= ( PI * 2.0f );
    Vec3 targetPosition = { 4.0f * cosf(angle), -1.0f, -5.0f };

    Vec3 moveNeeded = DiffVec(cameraPosition, targetPosition);
    Mat4 moveMat; SetToIdentity( &moveMat );
    SetTranslation( &moveMat, moveNeeded.x, moveNeeded.y, moveNeeded.z );

    Mat4 lookatMatrix = LookAtMatrix( targetPosition, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } );

    myCameraMatrix = lookatMatrix * moveMat * baseOrthoMat;

    return true;
}

void Render( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    glClear( GL_DEPTH_BUFFER_BIT );
    glBindFramebuffer( GL_FRAMEBUFFER, myFramebuffer.framebufferPtr );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, myFramebuffer.framebufferTexture.textureID, 0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SetSkeletonTransform( &frame1, &skeleton1 );
    RenderGLMeshBinding( &myCameraMatrix, &meshBinding1, &shader1, texSet1 );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    RenderFramebuffer( &myFramebuffer );
}