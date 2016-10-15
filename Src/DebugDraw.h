static char* PrimitiveVertShader = 
"#version 140\n"
"uniform mat4 cameraMatrix;"
"attribute vec3 position;"
"void main() { "
"    gl_Position = cameraMatrix * vec4( position, 1.0 );"
"}";

static char* PrimitiveFragShader =
"#version 140\n"
"uniform vec4 primitiveColor;"
"void main() {"
"    gl_FragColor = primitiveColor;"
"}";

struct PrimitiveDrawingData {
    PtrToGpuMem primitiveData;
    ShaderProgram primitiveShader;
    int transformUniformIndex;
    int cameraUniformIndex;
    int colorUniformIndex;
};

void DrawLine( 
    RenderDriver* renderDriver, 
    Vec3* line, 
    PrimitiveDrawingData* drawData,
    Mat4* cameraMatrix,
    float* color
) {
    renderDriver->CopyDataToGpuArray( 
        renderDriver,
        drawData->primitiveData, 
        (void*)line, 
        sizeof( Vec3 ) * 2 
    );

    Mat4 i; SetToIdentity( &i );

    float whiteColor[4] = { 1.0f,1.0f,1.0f,1.0f };
    if( color == NULL ) {
        color = (float*)whiteColor;
    }

    RenderCommand drawLineCommand = { };
    drawLineCommand.dolines = true;
    drawLineCommand.shader = &drawData->primitiveShader;
    drawLineCommand.elementCount = 2;
    drawLineCommand.vertexInputData[ 0 ] = drawData->primitiveData;
    //drawLineCommand.uniformData[ drawData->transformUniformIndex ] = &i;
    drawLineCommand.uniformData[ drawData->cameraUniformIndex ] = cameraMatrix;
    drawLineCommand.uniformData[ drawData->colorUniformIndex ] = color;

    renderDriver->DrawMesh( renderDriver, &drawLineCommand );
}

void DrawDot( 
    RenderDriver* renderDriver, 
    Vec3 p,
    float radius,
    PrimitiveDrawingData* drawData, 
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

    renderDriver->CopyDataToGpuArray(
        renderDriver,
        drawData->primitiveData,
        points,
        sizeof( Vec3 ) * numTrisInDot * 3
    );

    float myColor[4] = { 1.0f,1.0f,1.0f,1.0f };
    if( color == NULL ) {
        color = (float*)myColor;
    }

    RenderCommand drawDotCommand = { };
    drawDotCommand.dolines = false;
    drawDotCommand.shader = &drawData->primitiveShader;
    drawDotCommand.elementCount = numTrisInDot * 3;
    drawDotCommand.vertexInputData[ 0 ] = drawData->primitiveData;
    //drawDotCommand.uniformData[ drawData->transformUniformIndex ] = &i;
    drawDotCommand.uniformData[ drawData->cameraUniformIndex ] = cameraMatrix;
    drawDotCommand.uniformData[ drawData->colorUniformIndex ] = color;

    renderDriver->DrawMesh( renderDriver, &drawDotCommand );
}

void InitPrimitveRenderData( PrimitiveDrawingData* r, RenderDriver* renderDriver ) {
    r->primitiveData = renderDriver->AllocNewGpuArray( renderDriver );
    renderDriver->CreateShaderProgram( 
        renderDriver,
        PrimitiveVertShader,
        PrimitiveFragShader,
        &r->primitiveShader
    );

    /*r->transformUniformIndex = GetIndexOfProgramUniformInput( 
        &r->primitiveShader, 
        "modelMatrix" 
    );*/
    r->cameraUniformIndex = GetIndexOfProgramUniformInput(
        &r->primitiveShader,
        "cameraMatrix"
    );
    r->colorUniformIndex = GetIndexOfProgramUniformInput(
        &r->primitiveShader,
        "primitiveColor"
    );
}