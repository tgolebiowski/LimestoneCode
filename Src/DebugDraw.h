static char* PrimitiveVertShader = 
"#version 140\n"
"uniform mat4 cameraMatrix;"
"uniform mat4 modelMatrix;"
"attribute vec3 position;"
"void main() { "
"    gl_Position = cameraMatrix * modelMatrix * vec4( position, 1.0 );"
"}";

static char* PrimitiveFragShader =
"#version 140\n"
"uniform vec4 primitiveColor;"
"void main() {"
"    gl_FragColor = primitiveColor;"
"}";

struct PrimitiveDrawingData {
    RenderDriver* renderDriver;
    PtrToGpuMem primitiveData;
    ShaderProgram primitiveShader;
    int transformUniformIndex;
    int cameraUniformIndex;
    int colorUniformIndex;
};

static void DrawVertsImmediate( 
    PrimitiveDrawingData* primRenderer,
    Vec3* verts,
    int vertCount,
    bool line,
    bool suppressBackFaceCulling,
    Mat4* cameraMatrix,
    Mat4* modelMatrix,
    float* color
) {
    primRenderer->renderDriver->CopyDataToGpuArray( 
        primRenderer->primitiveData, 
        (void*)verts, 
        sizeof( Vec3 ) * vertCount 
    );

    Mat4 i; SetToIdentity( &i );
    if( modelMatrix == NULL ) {
        modelMatrix = &i;
    }

    float whiteColor[4] = { 1.0f,1.0f,1.0f,1.0f };
    if( color == NULL ) {
        color = (float*)whiteColor;
    }

    RenderCommand drawLineCommand = { };
    drawLineCommand.shader = &primRenderer->primitiveShader;
    drawLineCommand.elementCount = vertCount;
    drawLineCommand.vertexFormat = RenderCommand::SEPARATE_GPU_BUFFS;
    drawLineCommand.vertexInputData[ 0 ] = primRenderer->primitiveData;
    drawLineCommand.uniformData[ primRenderer->transformUniformIndex ] = modelMatrix;
    drawLineCommand.uniformData[ primRenderer->cameraUniformIndex ] = cameraMatrix;
    drawLineCommand.uniformData[ primRenderer->colorUniformIndex ] = color;

    primRenderer->renderDriver->Draw( &drawLineCommand, line, suppressBackFaceCulling );
}

static void DrawDot( 
    PrimitiveDrawingData* primRenderer,
    Vec3 p,
    float radius,
    Mat4* cameraMatrix,
    float* color
) {
    const int numTrisInDot = 14;
    Vec3 points [ numTrisInDot * 3 ];
    for( int i = 0; i < numTrisInDot; ++i ) {
        Vec3 edgeP1 = { 
            ( cos( ( 2 * PI * ((float)i) ) / (float)numTrisInDot ) * radius ) + p.x,
            ( sin( ( 2 * PI * ((float)i) ) / (float)numTrisInDot ) * radius ) + p.y,
            p.z
        };
        Vec3 edgeP2 = { 
            ( cos( ( 2 * PI * ((float)i + 1) ) / (float)numTrisInDot ) * radius ) + p.x,
            ( sin( ( 2 * PI * ((float)i + 1) ) / (float)numTrisInDot ) * radius ) + p.y,
            p.z
        };

        points[i * 3 + 0] = edgeP1;
        points[i * 3 + 1] = edgeP2;
        points[i * 3 + 2] = p;
    }

    primRenderer->renderDriver->CopyDataToGpuArray(
        primRenderer->primitiveData,
        points,
        sizeof( Vec3 ) * numTrisInDot * 3
    );

    float myColor[4] = { 1.0f,1.0f,1.0f,1.0f };
    if( color == NULL ) {
        color = (float*)myColor;
    }

    RenderCommand drawDotCommand = { };
    drawDotCommand.shader = &primRenderer->primitiveShader;
    drawDotCommand.elementCount = numTrisInDot * 3;
    drawDotCommand.vertexFormat = RenderCommand::SEPARATE_GPU_BUFFS;
    drawDotCommand.vertexInputData[ 0 ] = primRenderer->primitiveData;
    drawDotCommand.uniformData[ primRenderer->cameraUniformIndex ] = cameraMatrix;
    drawDotCommand.uniformData[ primRenderer->colorUniformIndex ] = color;

    primRenderer->renderDriver->Draw( &drawDotCommand, false, true );
}

static void DrawGridOnXZPlane( 
    PrimitiveDrawingData* primRenderer,
    Vec3 p, 
    float range, 
    Mat4* viewProjectionMat
) {
    Vec3 origin = { (int)p.x, 0.0f, (int)p.z };

    for( int i = 0; i < range; ++i ) {
        const float halfRange = range / 2.0f;

        float rangeX, minusRangeX, rangeZ, minusRangeZ;
        rangeX = origin.x - halfRange;
        minusRangeX = origin.x + halfRange;
        rangeZ = origin.z - halfRange;
        minusRangeZ = origin.z + halfRange;

        Vec3 x = { minusRangeX, 0.0f,  rangeZ + ( (float)i ) };
        Vec3 x2 = { rangeX, 0.0f, rangeZ + ( (float)i ) };

        Vec3 z = { rangeX + ( (float)i ), 0.0f, rangeZ };
        Vec3 z2 = { rangeX + ( (float)i ), 0.0f, minusRangeZ };

        Vec3 xAxis [2] = { x, x2 };
        Vec3 zAxis [2] = { z, z2 };

        float semiTransparentLine [4] = { 1.0f, 1.0f, 1.0f, 0.5f };
        DrawVertsImmediate( 
            primRenderer,
            (Vec3*)xAxis,
            2,
            true,
            false,
            viewProjectionMat,
            NULL,
            (float*)&semiTransparentLine
        );
        DrawVertsImmediate( 
            primRenderer,
            (Vec3*)zAxis,
            2,
            true,
            false,
            viewProjectionMat,
            NULL,
            (float*)&semiTransparentLine
        );
    }
}

static void InitPrimitveRenderData( 
    PrimitiveDrawingData* primRenderer,
    RenderDriver* renderDriver
) {
    primRenderer->primitiveData = renderDriver->AllocNewGpuArray();
    renderDriver->CreateShaderProgram( 
        PrimitiveVertShader,
        PrimitiveFragShader,
        &primRenderer->primitiveShader
    );

    primRenderer->transformUniformIndex = GetIndexOfProgramUniformInput( 
        &primRenderer->primitiveShader, 
        "modelMatrix" 
    );
    primRenderer->cameraUniformIndex = GetIndexOfProgramUniformInput(
        &primRenderer->primitiveShader,
        "cameraMatrix"
    );
    primRenderer->colorUniformIndex = GetIndexOfProgramUniformInput(
        &primRenderer->primitiveShader,
        "primitiveColor"
    );