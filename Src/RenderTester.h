struct TesterStorage {
    MeshGPUBinding renderBinding;
    ShaderProgramBinding shader;
    ShaderProgramParams programParams;
    Mat4 m1, m2;
    TextureBindingID greenTexBinding;
    TextureBindingID pinkTexBinding;

    Armature armature;
    Mat4 debugArmMat;
    Mat4 rotMat;
};

void InitRenderTester() {
	MeshGeometryData gData;
    MeshSkinningData sData;
    TextureData tData, pData;

    LoadMeshDataFromDisk( "Data/Pointy.dae", &gData );
    LoadMeshSkinningDataFromDisk( "Data/ComplexSkeletonDebug.dae", &sData, &gMem->armature );
    LoadTextureDataFromDisk( "Data/Textures/green_texture.png", &tData );
    LoadTextureDataFromDisk( "Data/Textures/pink_texture.png", &pData );

    CreateShaderProgram( &gMem->shader, "Data/Shaders/Vert.vert", "Data/Shaders/Frag.frag" );
    CreateRenderBinding( &gMem->renderBinding, &gData );
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

    Mat4 m;
    SetToIdentity( &m );
    SetRotation( &m, 1.0f, 0.0f, 0.0f, PI / 8.0f );
    *gMem->programParams.modelMatrix = MultMatrix( gMem->m1, m );

    SetToIdentity( &gMem->rotMat ); SetToIdentity( &gMem->debugArmMat );
    SetRotation( &gMem->rotMat, 0.0f, 1.0f, 0.0f, PI / 128.0f );
    SetScale( &gMem->debugArmMat, 0.25, 0.25, 0.25 );
    SetTranslation( &gMem->debugArmMat, 0.0f, -0.75, 0.0 );

}

void UpdateRenderTester() { 
    gMem->debugArmMat = MultMatrix( gMem->debugArmMat, gMem->rotMat );
}

void RenderTheTests() {
	RenderArmatureAsLines( &gMem->armature, gMem->debugArmMat );
}