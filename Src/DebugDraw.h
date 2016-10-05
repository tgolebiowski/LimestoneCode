struct DebugDraw {
	ShaderProgram pShader;
	//Data for rendering lines as a debugging tool
	uint32 lineDataPtr;
	uint32 lineIDataPtr;
    int32 pPosAttribPtr;
    int32 pCameraMatUniformPtr;
    int32 pColorUniformPtr;

	//Data for rendering circles/dots as a debugging tool
	uint32 circleDataPtr;
	uint32 circleIDataPtr;
};

static char* PrimitiveVertShader = 
"#version 140\n"
"uniform mat4 modelMatrix;"
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

static void RenderDebugCircle( 
    DebugDraw* debugDraw, 
    Vec3 position, 
    float radius, 
    Vec3 color 
) {
    #if 0
    glUseProgram( debugDraw->pShader.programID );

    Mat4 p; SetToIdentity( &p );
    SetTranslation( &p, position.x, position.y, position.z ); SetScale( &p, radius, radius, radius );
    assert( false );
    // glUniformMatrix4fv( rendererStorage->pShader.modelMatrixUniformPtr, 1, false, (float*)&p.m[0] );
    // glUniformMatrix4fv( rendererStorage->pShader.cameraMatrixUniformPtr, 1, false, (float*)&rendererStorage->cameraTransform.m[0] );
    // glUniform4f( glGetUniformLocation( rendererStorage->pShader.programID, "primitiveColor" ), color.x, color.y, color.z, 1.0f );

    // glEnableVertexAttribArray( rendererStorage->pShader.positionAttribute );
    // glBindBuffer( GL_ARRAY_BUFFER, rendererStorage->circleDataPtr );
    // glVertexAttribPointer( rendererStorage->pShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, rendererStorage->circleIDataPtr );
    // glDrawElements( GL_TRIANGLE_FAN, 18, GL_UNSIGNED_INT, NULL );
    #endif
}

static void DebugDraw_Lines( 
    void* glapi,
	DebugDraw* debugDraw, 
	float* vertexData, 
	uint8 dataCount, 
	Mat4* cameraMatrix, 
	Vec3 color 
) {
    #if 0
    GL_API* glApi = (GL_API*)glapi;
    glApi->glUseProgram( debugDraw->pShader.programID );

    float colorArray[4] = { color.x, color.y, color.z, 1.0f };
    glApi->glUniformMatrix4fv( debugDraw->pCameraMatUniformPtr, 1, false, (float*)cameraMatrix );
    glApi->glUniform4fv( debugDraw->pColorUniformPtr, 1, (float*)&colorArray );

    glApi->glEnableVertexAttribArray( debugDraw->pPosAttribPtr );
    glApi->glBindBuffer( GL_ARRAY_BUFFER, debugDraw->lineDataPtr );
    glApi->glBufferData( GL_ARRAY_BUFFER, dataCount * 3 * sizeof(float), (GLfloat*)vertexData, GL_DYNAMIC_DRAW );
    glApi->glVertexAttribPointer( debugDraw->pPosAttribPtr, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glApi->glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, debugDraw->lineIDataPtr );
    glApi->glDrawElements( GL_LINES, dataCount, GL_UNSIGNED_INT, NULL );
    #endif
}

static void DebugDraw_3DVectorArrow( 
    void* glapi,
	DebugDraw* debugDraw, 
	Vec3 origin, 
	Quat rotation, 
	Mat4 cameraMatrix 
) {
    #if 0
    GL_API* glApi = (GL_API*)glapi;
    Vec3 forward = ApplyQuatToVec( rotation, { 0.0f, 0.0f, 1.0f } );
    Vec3 up = ApplyQuatToVec( rotation, { 0.0f, 1.0f, 0.0f } );

    Vec3 upline [2] = {
        origin,
        origin + up * 0.5f
    };
    Vec3 forwardLine [2] = {
        origin,
        origin + forward * 1.2f 
    };

    RenderDebugLines( debugDraw, (float*)upline, 2, &cameraMatrix, { 0.77, 0.77, 0.77 } );
    RenderDebugLines( debugDraw, (float*)forwardLine, 2, &cameraMatrix, { 1.0, 1.0, 1.0 } );
    #endif
}

#if 0
static void RenderArmatureAsLines( 
    RendererStorage* rStorage,
    DebugDraw* debugDraw,
    Armature* armature, 
    Mat4 transform, 
    Vec3 color 
) {
    bool isDepthTesting;
    glGetBooleanv( GL_DEPTH_TEST, ( GLboolean* )&isDepthTesting );
    if( isDepthTesting ) {
        glDisable( GL_DEPTH_TEST );
    }

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

    if( isDepthTesting ) {
        glEnable( GL_DEPTH_TEST );
    }
}
#endif

void InitDebugDraw( DebugDraw* debugDraw ) {
    #if 0

    CreateShaderProgram( 
        PrimitiveVertShader, 
        PrimitiveFragShader, 
        &debugDraw->pShader 
    );

    //Initialization of data for line primitives
    GLuint glLineDataPtr, glLineIndexPtr;
    glGenBuffers( 1, &glLineDataPtr );
    glGenBuffers( 1, &glLineIndexPtr );
    debugDraw->lineDataPtr = glLineDataPtr;
    debugDraw->lineIDataPtr = glLineIndexPtr;
    debugDraw->pPosAttribPtr = GetShaderProgramInputPtr( &debugDraw->pShader, "position" );
    debugDraw->pCameraMatUniformPtr = GetShaderProgramInputPtr( &debugDraw->pShader, "cameraMatrix" );
    debugDraw->pColorUniformPtr = GetShaderProgramInputPtr( &debugDraw->pShader, "primitiveColor" );

    GLuint sequentialIndexBuffer[64];
    for( uint8 i = 0; i < 64; i++ ) {
        sequentialIndexBuffer[i] = i;
    }
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, glLineIndexPtr );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 64 * sizeof(GLuint), &sequentialIndexBuffer[0], GL_STATIC_DRAW );

    //Initialization of data for circle primitives
    GLuint glCircleDataPtr, glCircleIndexPtr;
    glGenBuffers( 1, &glCircleDataPtr );
    glGenBuffers( 1, &glCircleIndexPtr );
    debugDraw->circleDataPtr = glCircleDataPtr;
    debugDraw->circleIDataPtr = glCircleIndexPtr;

    GLfloat circleVertexData[ 18 * 3 ];
    circleVertexData[0] = 0.0f;
    circleVertexData[1] = 0.0f;
    circleVertexData[2] = 0.0f;
    for( uint8 i = 0; i < 17; i++ ) {
        GLfloat x, y;
        float angle = (2.0f * PI / 16.0f) * (float)i;
        x = cosf( angle );
        y = sinf( angle );
        circleVertexData[ (i + 1) * 3 ] = x;
        circleVertexData[ (i + 1) * 3 + 1 ] = y;
        circleVertexData[ (i + 1) * 3 + 2 ] = 0.0f;
    }
    glBindBuffer( GL_ARRAY_BUFFER, glCircleDataPtr );
    glBufferData( GL_ARRAY_BUFFER, 18 * 3 * sizeof(GLfloat), &circleVertexData[0], GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, glCircleIndexPtr );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 18 * sizeof(GLuint), &sequentialIndexBuffer[0], GL_STATIC_DRAW );

    #endif    
}