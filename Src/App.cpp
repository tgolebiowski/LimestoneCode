struct GameMemory {
    MeshGPUBinding renderBinding;
    ShaderProgramBinding shader;
    ShaderProgramParams programParams;
    Mat4 m1, m2;
    TextureBindingID greenTexBinding;
    TextureBindingID pinkTexBinding;
};

static Mat4 rotMat;

void GameInit( MemorySlab* gameMemory ) {
    if( InitRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) == false ) {
        printf("Failed to init renderer\n");
        return;
    }

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    Bone bone;

    MeshGeometryStorage data;
    MeshSkinningStorage sData;
    Armature armature;
    TextureDataStorage tData, pData;
    LoadMeshDataFromDisk( "Data/Pointy.dae", &data );
    LoadMeshSkinningDataFromDisk( "Data/ComplexSkeletonDebug.dae", &sData, &armature );
    LoadTextureDataFromDisk( "Data/Textures/green_texture.png", &tData );
    LoadTextureDataFromDisk( "Data/Textures/pink_texture.png", &pData );

    CreateShaderProgram( &gMem->shader, "Data/Shaders/Vert.vert", "Data/Shaders/Frag.frag" );
    CreateRenderBinding( &gMem->renderBinding, &data );
    CreateTextureBinding( &gMem->greenTexBinding, &tData );
    CreateTextureBinding( &gMem->pinkTexBinding, &pData );

    ShaderProgramParams* params = &gMem->programParams;
    params->modelMatrix = &gMem->m1; 
    params->cameraMatrix = &gMem->m2;
    SetToIdentity( params->modelMatrix );
    SetScale( params->modelMatrix, 0.33, 0.33, 0.33 );
    SetToIdentity( params->cameraMatrix );
    params->sampler1 = gMem->greenTexBinding;
    params->sampler2 = gMem->pinkTexBinding;

    SetToIdentity( &rotMat );
    SetRotation( &rotMat, 0.0f, 1.0f, 0.0f, 3.1415926f / 128.0f );

    Mat4 m;
    SetToIdentity( &m );
    SetRotation( &m, 1.0f, 0.0f, 0.0f, 3.1415926 / 8.0f );
    *gMem->programParams.modelMatrix = MultMatrix( gMem->m1, m );

    return;
}

bool Update( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    *gMem->programParams.modelMatrix = MultMatrix( *gMem->programParams.modelMatrix, rotMat );

    return true;
}

void Render( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    //glClear( GL_DEPTH_BUFFER_BIT );
    //glBindFramebuffer( GL_FRAMEBUFFER, myFramebuffer.framebufferPtr );
    //glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, myFramebuffer.framebufferTexture.textureID, 0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderBoundData( &gMem->renderBinding, &gMem->shader, gMem->programParams );

    //RenderGLMeshBinding( &baseOrthoMat, &meshBinding1, &shader1, texSet1 );
    //RenderSkeletonAsLines( &skeleton1, &meshData1.modelMatrix );

    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //RenderFramebuffer( &myFramebuffer );
}