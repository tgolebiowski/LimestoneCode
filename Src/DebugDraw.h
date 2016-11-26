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

static void MakeIcosphere( MeshGeometryData* data, Stack* allocater ) {
    float t = ( 1.0f + sqrt( 5.0f ) ) / 2.0f;

    Vec3 basePoints [12] = {
        { -1.0f, t, 0.0f },
        { 1.0f, t, 0.0f },
        { -1.0f, -t, 0.0f },
        { 1.0f, -t, 0.0f },

        { 0.0f, -1.0f, t },
        { 0.0f, 1.0f, t },
        { 0.0f, -1.0f, -t },
        { 0.0f, 1.0f, -t },

        { t, 0.0f, -1.0f },
        { t, 0.0f, 1.0f },
        { -t, 0.0f, -1.0f },
        { -t, 0.0f, 1.0f }
    };

    float radius = 0.5f;
    for( int i = 0; i < 12; ++i ) {
        Normalize( &basePoints[ i ] );
        basePoints[i] = basePoints[ i ] * radius;
    }

    int indexList [ 20 * 3 ] = {
        0, 11, 5,
        0, 5, 1,
        0, 1, 7,
        0, 7, 10,
        0, 10, 11,

        1, 5, 9,
        5, 11, 4,
        11, 10, 2,
        10, 7, 6,
        7, 1, 8,

        3, 9, 4,
        3, 4, 2,
        3, 2, 6,
        3, 6, 8,
        3, 8, 9,

        4, 9, 5,
        2, 4, 11,
        6, 2, 10,
        8, 6, 7,
        9, 8, 1
    };

    data->vData = (Vec3*)StackAllocAligned( allocater, sizeof( Vec3 ) * 20 * 3 );

    for( int i = 0; i < 20 * 3; ++i ) {
        data->vData[ i ] = basePoints[ indexList[ i ] ];
    }

    data->dataCount = 20 * 3;
}

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
    drawLineCommand.vertexFormat = RenderCommand::SEPARATE_GPU_ARRAYS;
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

    Mat4 matrix = {}; SetToIdentity( &matrix );

    RenderCommand drawDotCommand = { };
    drawDotCommand.shader = &primRenderer->primitiveShader;
    drawDotCommand.elementCount = numTrisInDot * 3;
    drawDotCommand.vertexFormat = RenderCommand::SEPARATE_GPU_ARRAYS;
    drawDotCommand.vertexInputData[ 0 ] = primRenderer->primitiveData;
    drawDotCommand.uniformData[ primRenderer->cameraUniformIndex ] = cameraMatrix;
    drawDotCommand.uniformData[ primRenderer->transformUniformIndex ] = (void*)&matrix;
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

    primRenderer->renderDriver = renderDriver;
}

//Weird idea that didn't work:
//Take tetrahedron, subdivide faces into a "triforce", push verticies are not the radius
//outwards, this may "approach sphereicallness"
#if 0
    //Base Tetrahedron data
    float myRad = 1.0f;
    Vec3 v1, v2, v3, v4;
    v1 = { 0.0f, 1.0f * myRad, 0.0f };
    v2 = { 0.943f * myRad, -0.333f * myRad , 0.0f };
    v3 = { -0.471f * myRad, -0.333f * myRad, 0.816f * myRad };
    v4 = { -0.471f * myRad, -0.333f * myRad, -0.816f * myRad };
    Vec3 basePoints [12] = {
        v1, v2, v3,
        v4, v3, v2,
        v1, v4, v2,
        v1, v3, v4
    };

    data->vData = (Vec3*)StackAllocAligned( allocater, sizeof( Vec3 ) * 12 * 4 );
    data->normalData = (Vec3*)StackAllocAligned( allocater, sizeof( Vec3 ) * 12 * 4 );

    memcpy( data->vData, basePoints, sizeof( Vec3 ) * 12 );
    memcpy( data->normalData, basePoints, sizeof( Vec3 ) * 12 );
    data->dataCount = 12;


    for( int i = 0; i < 4; ++i ) {
        Vec3 baseFacePoints [3];
        baseFacePoints[0] = basePoints[ 0 + i * 3 ];
        baseFacePoints[1] = basePoints[ 1 + i * 3 ];
        baseFacePoints[2] = basePoints[ 2 + i * 3 ];

        //Create each of the faces that touch one of the original points
        for( int j = 0; j < 3; ++j ) {
            Vec3 p = baseFacePoints[ j ];
            Vec3 p1 = baseFacePoints[ abs( ( j - 1 ) % 3 ) ];
            Vec3 p2 = baseFacePoints[ ( j + 1 ) % 3 ];

            Vec3 newP1 = ( (p1 - p) * 0.5f ) + p;
            Normalize( &newP1 );
            newP1 = newP1 * myRad;

            Vec3 newP2 = ( (p2 - p) * 0.5f ) + p;
            Normalize( &newP2 );
            newP2 = newP2 * myRad;

            int baseIndex = i * 12 + j * 3;
            data->vData[ baseIndex + 0 ] = newP1;
            data->vData[ baseIndex + 1 ] = p;
            data->vData[ baseIndex + 2 ] = newP2;

            Vec3 n = Cross( newP1 - p, newP2 - p );
            Normalize( &n );
            data->normalData[ baseIndex + 0 ] = n;
            data->normalData[ baseIndex + 1 ] = n;
            data->normalData[ baseIndex + 2 ] = n;
        }

        //Create the middle face
        {
            int baseIndex = i * 12 + 9;
            for( int j = 0; j < 3; ++j ) {
                Vec3 midP = { };
                Vec3 midP1 = baseFacePoints[ j ];
                Vec3 midP2 = baseFacePoints[ ( j + 1 ) % 3 ];
                midP = ( ( midP1 - midP2 ) * 0.5f ) + midP2;

                Normalize( &midP );
                midP = midP * myRad;

                data->vData[ baseIndex + j ] = midP;
                data->normalData[ baseIndex + j ] = midP;
                Normalize( &data->normalData[ baseIndex ] );
            }
        }
    }

    data->dataCount = 48;
#endif