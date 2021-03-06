static char* PrimitiveVertShader = 
"#version 140\n"
"uniform mat4 cameraMatrix;"
"uniform mat4 modelMatrix;"
"attribute vec3 position;"
"attribute vec3 color;"

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

static void MakeIcosphere( 
    MeshGeometryData* data, 
    Stack* allocater, 
    float radius = 0.5f 
) {
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

    for( int i = 0; i < 12; ++i ) {
        Normalize( &basePoints[ i ] );
        basePoints[i] = basePoints[ i ] * radius;
        assert( abs( Vec3Length( basePoints[i] ) - radius ) < 1e-04 );
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

    data->normalData = (Vec3*)StackAllocAligned( allocater, sizeof( Vec3 ) * 20 * 3 );
    for( int i = 0; i < 20; ++i ) {
        int baseIndex = i * 3;
        Vec3 v1 = basePoints[ indexList[ baseIndex + 0 ] ];
        Vec3 v2 = basePoints[ indexList[ baseIndex + 1 ] ];
        Vec3 v3 = basePoints[ indexList[ baseIndex + 2 ] ];

        Vec3 n = Cross( ( v3 - v2 ), ( v1 - v2 ) );
        Normalize( &n );

        float dotTest = Dot( v3, n );
        if( dotTest < 0.0f ) {
            n = n * -1.0f;
        }

        data->normalData[ baseIndex + 0 ] = n;
        data->normalData[ baseIndex + 1 ] = n;
        data->normalData[ baseIndex + 2 ] = n;
    }

    data->dataCount = 20 * 3;
}

static void MakeTexturedQuad( 
    MeshGeometryData* data, 
    Stack* allocater, 
    float x, 
    float y 
) {
    float halfX = 0.5f * x;
    float halfY = 0.5f * y;

    Vec3 verts [4] = {
        { -halfX, halfY, 0.0f },
        { halfX, halfY, 0.0f },
        { halfX, -halfY, 0.0f },
        { -halfX, -halfY, 0.0f }
    };
    Vec2 uv [4] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };

    data->vData = (Vec3*)StackAllocAligned( allocater, sizeof( Vec3 ) * 2 * 3 );
    data->normalData = (Vec3*)StackAllocAligned( allocater, sizeof( Vec3 ) * 2 * 3 );
    data->uvData = (Vec2*)StackAllocAligned( allocater, sizeof( Vec2 ) * 2 * 3 );

    //CCW indicies
    const int indexBuffer [6] = { 0,3,2,0,2,1 };

    for( int i = 0; i < 6; ++i ) {
        data->vData[ i ] = verts[ indexBuffer[ i ] ];
        data->uvData[ i ] = uv[ indexBuffer[ i ] ];
        data->normalData[ i ] = { 0.0f, 0.0f, 1.0f };
    }
    data->dataCount = 6;
    data->aabbMax = { halfX, halfY, 0.0f };
    data->aabbMin = { -halfX, -halfY, 0.0f };
}

static void DrawVertsImmediate( 
    PrimitiveDrawingData* primRenderer,
    Vec3* verts,
    int vertCount,
    bool line,
    bool suppressBackFaceCulling,
    bool wireframe,
    bool nodepthtest,
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

    if( suppressBackFaceCulling ) { 
        primRenderer->renderDriver->SetRenderState( SUPPRESS_BACKFACE_CULL, 0.0f ); 
    }
    if( nodepthtest ) { 
        primRenderer->renderDriver->SetRenderState( SUPPRESS_DEPTH_TEST, 0.0f ); 
    }
    primRenderer->renderDriver->Draw( &drawLineCommand, line, wireframe );
    primRenderer->renderDriver->ResetRenderState();
}

static void DrawDot( 
    PrimitiveDrawingData* primRenderer,
    Vec3 p,
    float radius,
    Mat4* cameraMatrix,
    Mat4* modelMatrix,
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
    drawDotCommand.vertexFormat = RenderCommand::SEPARATE_GPU_ARRAYS;
    drawDotCommand.vertexInputData[ 0 ] = primRenderer->primitiveData;
    drawDotCommand.uniformData[ primRenderer->cameraUniformIndex ] = cameraMatrix;
    drawDotCommand.uniformData[ primRenderer->transformUniformIndex ] = (void*)modelMatrix;
    drawDotCommand.uniformData[ primRenderer->colorUniformIndex ] = color;

    primRenderer->renderDriver->SetRenderState( SUPPRESS_DEPTH_WRITE, 0.0f );
    primRenderer->renderDriver->Draw( &drawDotCommand, false, false );
    primRenderer->renderDriver->ResetRenderState();
}

static void DrawGridOnXZPlane( 
    PrimitiveDrawingData* primRenderer,
    Vec3 p, 
    float range, 
    Mat4* viewProjectionMat
) {
    Vec3 origin = { (int)p.x, 0.0f, (int)p.z };

    for( int i = 0; i <= range; ++i ) {
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
            false,
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
            false,
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

    primRenderer->transformUniformIndex = GetIndexOfShaderInput( 
        &primRenderer->primitiveShader, 
        "modelMatrix" 
    );
    primRenderer->cameraUniformIndex = GetIndexOfShaderInput(
        &primRenderer->primitiveShader,
        "cameraMatrix"
    );
    primRenderer->colorUniformIndex = GetIndexOfShaderInput(
        &primRenderer->primitiveShader,
        "primitiveColor"
    );

    primRenderer->renderDriver = renderDriver;
}

static void RenderArmatureAsLines( 
    PrimitiveDrawingData* primRenderer, 
    Mat4 cameraMatrix,
    Armature* armature
) {
    //TODO: Move this logic to debugdraw
    struct LocalFunctions {
        Mat4* cameraMatrix;
        Mat4* modelMatrix;
        void drawBone(Bone* bone, PrimitiveDrawingData* primRenderer) {
            Vec3 origin = { 0,0,0 };
            //origin = MultVec( bone->invBindPose, origin );
            origin = TransformVec( bone->transform, origin );

            if( bone->childCount > 0 ) {
                for( int i = 0; i < bone->childCount; ++i ) {
                    Bone* childBone = bone->children[ i ];
                    Vec3 o2 = { 0,0,0 };
                    //o2 = MultVec( childBone->invBindPose, o2 );
                    o2 = TransformVec( childBone->transform, o2 );

                    Vec3 v [2] = { origin, o2 };
                    DrawVertsImmediate(
                        primRenderer,
                        v, 2, true, false, false, false,
                        this->cameraMatrix,
                        this->modelMatrix,
                        NULL
                    );
                    this->drawBone( childBone, primRenderer );
                }
            } else {
                Vec3 o2 = { 0, 1, 0 };
                o2 = TransformVec( bone->transform, o2 );

                Vec3 v [2] = { origin, o2 };
                DrawVertsImmediate(
                    primRenderer,
                    v, 2, true, false, false, false,
                    this->cameraMatrix,
                    this->modelMatrix,
                    NULL
                );
            }
        };
    } lf = {
        &cameraMatrix,
        &IdentityMatrix
    };
    lf.drawBone( armature->rootBone, primRenderer );
}

#if 0 
static void RenderArmatureAsLines( 
    RendererStorage* rStorage,
    DebugDraw* debugDraw,
    Armature* armature, 
    Mat4 transform, 
    Vec3 color 
) {
    /*bool isDepthTesting;
    glGetBooleanv( GL_DEPTH_TEST, ( GLboolean* )&isDepthTesting );
    if( isDepthTesting ) {
        glDisable( GL_DEPTH_TEST );
    }*/

    uint8 dataCount = 0;
    uint8 jointData = 0;
    Vec3 boneLines [64];
    Vec3 jointXAxes [64];
    Vec3 jointYAxes [64];
    Vec3 jointZAxes [64];

    for( uint8 boneIndex = 0; boneIndex < armature->boneCount; ++boneIndex ) {
        Bone* bone = &armature->bones[ boneIndex ];

        Vec3 p1 = { 0.0f, 0.0f, 0.0f };
        p1 = MultVec( InverseMatrix( bone->invBindPose ), p1 );
        p1 = MultVec( *bone->currentTransform, p1 );
        RenderDebugCircle( debugDraw, MultVec( transform, p1 ), 0.05f, { 1.0f, 1.0f, 1.0f } );

        if( bone->childCount > 0 ) {
            for( uint8 childIndex = 0; childIndex < bone->childCount; ++childIndex ) {
                Bone* child = bone->children[ childIndex ];

                Vec3 p2 = { 0.0f, 0.0f, 0.0f };
                p2 = MultVec( InverseMatrix( child->invBindPose ), p2 );
                p2 = MultVec( *child->currentTransform, p2 );

                boneLines[ dataCount++ ] = p1;
                boneLines[ dataCount++ ] = p2;
            }
        }

        Vec3 px = { 1.0f, 0.0f, 0.0f };
        Vec3 py = { 0.0f, 1.0f, 0.0f };
        Vec3 pz = { 0.0f, 0.0f, 1.0f };
        px = MultVec( InverseMatrix( bone->invBindPose ), px );
        px = MultVec( *bone->currentTransform, px );
        py = MultVec( InverseMatrix( bone->invBindPose ), py );
        py = MultVec( *bone->currentTransform, py );
        pz = MultVec( InverseMatrix( bone->invBindPose ), pz );
        pz = MultVec( *bone->currentTransform, pz );
        jointXAxes[ jointData ] = p1; 
        jointYAxes[ jointData ] = p1; 
        jointZAxes[ jointData ] = p1;
        jointData++;
        jointXAxes[ jointData ] = px;
        jointYAxes[ jointData ] = py;
        jointZAxes[ jointData ] = pz;
        jointData++;
    }

    //RenderDebugLines( rStorage, (float*)&boneLines[0], dataCount, transform, color );
    //RenderDebugLines( rStorage, (float*)&jointXAxes[0], jointData, transform, { 0.09, 0.85, 0.15 } );
    //RenderDebugLines( rStorage, (float*)&jointYAxes[0], jointData, transform, { 0.85, 0.85, 0.14 } );
    //RenderDebugLines( rStorage, (float*)&jointZAxes[0], jointData, transform, { 0.09, 0.11, 0.85 } );

    //if( isDepthTesting ) {
        //glEnable( GL_DEPTH_TEST );
    //}
}
#endif

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