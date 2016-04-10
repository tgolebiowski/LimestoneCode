bool InitRenderer( uint16 screen_w, uint16 screen_h ) {
	printf( "Vendor: %s\n", glGetString( GL_VENDOR ) );
    printf( "Renderer: %s\n", glGetString( GL_RENDERER ) );
    printf( "GL Version: %s\n", glGetString( GL_VERSION ) );
    printf( "GLSL Version: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

    GLint k;
    glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &k );
    printf("Max texture units: %d\n", k);

    glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &k );
    printf( "Max Vertex Attributes: %d\n", k );

    //Initialize clear color
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glViewport( 0.0f, 0.0f, screen_w, screen_h );

    SetToIdentity( &rendererStorage.baseProjectionMatrix );
    SetToIdentity( &rendererStorage.cameraTransform );

    //Initialize framebuffer
    //CreateFrameBuffer( &myFramebuffer );
    //CreateShader( &framebufferShader, "Data/Shaders/Framebuffer.vert", "Data/Shaders/Framebuffer.frag" );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR ) {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        return false;
    } else {
        printf( "Initialized OpenGL\n" );
    }

    CreateShaderProgram( &rendererStorage.pShader, "Data/Shaders/Primitive.vert", "Data/Shaders/Primitive.frag" );

    //Initialization of data for line primitives
    GLuint glLineDataPtr, glLineIndexPtr;
    glGenBuffers( 1, &glLineDataPtr );
    glGenBuffers( 1, &glLineIndexPtr );
    rendererStorage.lineDataPtr = glLineDataPtr;
    rendererStorage.lineIDataPtr = glLineIndexPtr;

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
    rendererStorage.circleDataPtr = glCircleDataPtr;
    rendererStorage.circleIDataPtr = glCircleIndexPtr;

    GLfloat circleVertexData[ 18 * 3 ];
    circleVertexData[0] = 0.0f;
    circleVertexData[1] = 0.0f;
    circleVertexData[2] = 0.0f;
    for( uint8 i = 0; i < 17; i++ ) {
        GLfloat x, y;
        float angle = (2 * PI / 16.0f) * (float)i;
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

    return true;
}

void CreateTextureBinding( TextureBindingID* texBindID, TextureData* textureData ) {
	GLenum pixelFormat;
    if( textureData->channelsPerPixel == 3 ) {
        pixelFormat = GL_RGB;
    } else if( textureData->channelsPerPixel == 4 ) {
        pixelFormat = GL_RGBA;
    }

    GLuint glTextureID;
    glGenTextures( 1, &glTextureID );
    glBindTexture( GL_TEXTURE_2D, glTextureID );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    glTexImage2D( GL_TEXTURE_2D, 0, textureData->channelsPerPixel, textureData->width, 
    	textureData->height, 0, pixelFormat, GL_UNSIGNED_BYTE, textureData->texData );

    *texBindID = glTextureID;

    //stbi_image_free( data );
}

void PrintGLShaderLog( GLuint shader ) {
    //Make sure name is shader
    if( glIsShader( shader ) ) {
        //Shader log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        
        //Get info string length
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
        
        //Allocate string
        GLchar infoLog[ 256 ];
        
        //Get info log
        glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
        if( infoLogLength > 0 ) {
            //Print Log
            printf( "%s\n", infoLog );
        }
    } else {
        printf( "Name %d is not a shader\n", shader );
    }
}

void CreateShaderProgram( ShaderProgramBinding* bindDataStorage, const char* vertProgramFilePath, const char* fragProgramFilePath ) {
    const size_t shaderSrcBufferLength = 1700;
    char shaderSrcBuffer[shaderSrcBufferLength];
    char* bufferPtr = (char*)&shaderSrcBuffer[0];
    memset( bufferPtr, 0, sizeof(char) * shaderSrcBufferLength );

    bindDataStorage->programID = glCreateProgram();

    GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
    //TODO: Error handling on this
    uint16_t readReturnCode = ReadShaderSrcFileFromDisk( vertProgramFilePath, bufferPtr, shaderSrcBufferLength );
    assert(readReturnCode == 0);
    glShaderSource( vertexShader, 1, &bufferPtr, NULL );

    glCompileShader( vertexShader );
    GLint compiled = GL_FALSE;
    glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Could not compile Vertex Shader\n" );
        PrintGLShaderLog( vertexShader );
    } else {
        printf( "Vertex Shader %s compiled\n", vertProgramFilePath );
        glAttachShader( bindDataStorage->programID, vertexShader );
    }

    memset( bufferPtr, 0, sizeof(char) * shaderSrcBufferLength );

    GLuint fragShader = glCreateShader( GL_FRAGMENT_SHADER );
    //TODO: Error handling on this
    readReturnCode = ReadShaderSrcFileFromDisk( fragProgramFilePath, bufferPtr, shaderSrcBufferLength );
    assert(readReturnCode == 0);
    glShaderSource( fragShader, 1, &bufferPtr, NULL );

    glCompileShader( fragShader );
    //Check for errors
    compiled = GL_FALSE;
    glGetShaderiv( fragShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Unable to compile fragment shader\n" );
        PrintGLShaderLog( fragShader );
    } else {
        printf( "Frag Shader %s compiled\n", fragProgramFilePath );
        //Actually attach it if it compiled
        glAttachShader( bindDataStorage->programID, fragShader );
    }

    glLinkProgram( bindDataStorage->programID );
    //Check for errors
    compiled = GL_TRUE;
    glGetProgramiv( bindDataStorage->programID, GL_LINK_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Error linking program\n" );
    } else {
        printf( "Shader Program Linked Successfully\n");
    }

    glUseProgram(bindDataStorage->programID);

    bindDataStorage->modelMatrixUniformPtr = glGetUniformLocation( bindDataStorage->programID, "modelMatrix" );
    bindDataStorage->cameraMatrixUniformPtr = glGetUniformLocation( bindDataStorage->programID, "cameraMatrix" );
    bindDataStorage->isArmatureAnimated = glGetUniformLocation( bindDataStorage->programID, "isSkeletalAnimated" );
    bindDataStorage->armatureUniformPtr = glGetUniformLocation( bindDataStorage->programID, "skeleton" );

    bindDataStorage->positionAttribute = glGetAttribLocation( bindDataStorage->programID, "position" );
    bindDataStorage->texCoordAttribute = glGetAttribLocation( bindDataStorage->programID, "texCoord" );
    bindDataStorage->boneWeightsAttribute = glGetAttribLocation( bindDataStorage->programID, "boneWeights" );
    bindDataStorage->boneIndiciesAttribute = glGetAttribLocation( bindDataStorage->programID, "boneIndicies" );

    bindDataStorage->samplerPtr1 = glGetUniformLocation( bindDataStorage->programID, "tex1" );
    bindDataStorage->samplerPtr2 = glGetUniformLocation( bindDataStorage->programID, "tex2" );
    glUniform1i( bindDataStorage->samplerPtr1, 0 );
    glUniform1i( bindDataStorage->samplerPtr2, 1 );

    glUseProgram(0);
    glDeleteShader( vertexShader ); 
    glDeleteShader( fragShader );
}

void CreateRenderBinding( MeshGPUBinding* bindDataStorage, MeshGeometryData* meshDataStorage ) {
	GLuint glVBOPtr;
	glGenBuffers( 1, &glVBOPtr );
	glBindBuffer( GL_ARRAY_BUFFER, glVBOPtr );
	glBufferData( GL_ARRAY_BUFFER, meshDataStorage->dataCount * 3 * sizeof(float), &meshDataStorage->vData, GL_STATIC_DRAW );
	bindDataStorage->vertexDataPtr = glVBOPtr;

	GLuint glIBOPtr;
	glGenBuffers( 1, &glIBOPtr );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, glIBOPtr );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, meshDataStorage->dataCount * sizeof(uint32), &meshDataStorage->iData, GL_STATIC_DRAW );
	bindDataStorage->indexDataPtr = glIBOPtr;

	GLuint glUVPtr;
	glGenBuffers( 1, &glUVPtr );
	glBindBuffer( GL_ARRAY_BUFFER, glUVPtr );
	glBufferData( GL_ARRAY_BUFFER, meshDataStorage->dataCount * 2 * sizeof(float), &meshDataStorage->uvData, GL_STATIC_DRAW );
	bindDataStorage->uvDataPtr = glUVPtr;

    if( meshDataStorage->hasBoneData ) {
        bindDataStorage->hasBoneData = true;

        GLuint glBoneWeightBufferPtr;
        glGenBuffers( 1, &glBoneWeightBufferPtr );
        glBindBuffer( GL_ARRAY_BUFFER, glBoneWeightBufferPtr );
        glBufferData( GL_ARRAY_BUFFER, meshDataStorage->dataCount * MAXBONESPERVERT * sizeof(float), &meshDataStorage->boneWeightData, GL_STATIC_DRAW );
        bindDataStorage->boneWeightDataPtr = glBoneWeightBufferPtr;

        GLuint glBoneIndexBufferPtr;
        glGenBuffers( 1, &glBoneIndexBufferPtr );
        glBindBuffer( GL_ARRAY_BUFFER, glBoneIndexBufferPtr );
        glBufferData( GL_ARRAY_BUFFER, meshDataStorage->dataCount * MAXBONESPERVERT * sizeof(uint32), &meshDataStorage->boneIndexData, GL_STATIC_DRAW );
        bindDataStorage->boneIndexDataPtr = glBoneIndexBufferPtr;
    } else {
        bindDataStorage->hasBoneData = false;
    }

	bindDataStorage->dataCount = meshDataStorage->dataCount;
}

void RenderBoundData( MeshGPUBinding* meshBinding, ShaderProgramBinding* programBinding, ShaderProgramParams params ) {
	//Flush errors
    //while( glGetError() != GL_NO_ERROR ){};

    //Bind Shader
    glUseProgram( programBinding->programID );
    glUniformMatrix4fv( programBinding->modelMatrixUniformPtr, 1, false, (float*)&params.modelMatrix->m[0] );
    glUniformMatrix4fv( programBinding->cameraMatrixUniformPtr, 1, false, (float*)&rendererStorage.baseProjectionMatrix.m[0] );
    //glUniform1i( program->isArmatureAnimatedUniform, (int)mesh->isArmatureAnimated );

    //Set vertex data
    glBindBuffer( GL_ARRAY_BUFFER, meshBinding->vertexDataPtr );
    glEnableVertexAttribArray( programBinding->positionAttribute );
    glVertexAttribPointer( programBinding->positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set UV data
    glBindBuffer( GL_ARRAY_BUFFER, meshBinding->uvDataPtr );
    glEnableVertexAttribArray( programBinding->texCoordAttribute );
    glVertexAttribPointer( programBinding->texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    if( meshBinding->hasBoneData && params.armature != NULL ) {
        glUniform1i( programBinding->isArmatureAnimated, 1 );
        glUniformMatrix4fv( programBinding->armatureUniformPtr, MAXBONES, GL_FALSE, (float*)&params.armature->boneTransforms->m[0] );

        glBindBuffer( GL_ARRAY_BUFFER, meshBinding->boneWeightDataPtr );
        glEnableVertexAttribArray( programBinding->boneWeightsAttribute );
        glVertexAttribPointer( programBinding->boneWeightsAttribute, MAXBONESPERVERT, GL_FLOAT, false, 0, 0 );

        glBindBuffer( GL_ARRAY_BUFFER, meshBinding->boneIndexDataPtr );
        glEnableVertexAttribArray( programBinding->boneIndiciesAttribute );
        glVertexAttribIPointer( programBinding->boneIndiciesAttribute, MAXBONESPERVERT, GL_UNSIGNED_INT, 0, 0 );
    } else {
        glUniform1i( programBinding->isArmatureAnimated, 0 );
    }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, params.sampler1 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, params.sampler2 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, meshBinding->indexDataPtr );
    glDrawElements( GL_TRIANGLES, meshBinding->dataCount, GL_UNSIGNED_INT, NULL );

    //Unbind textures
    //glBindTexture( GL_TEXTURE_2D, 0 );
    //glActiveTexture( GL_TEXTURE0 );
    //glBindTexture( GL_TEXTURE_2D, 0 );
    //clear shader
    //glUseProgram( (GLuint)NULL );
}

void RenderDebugCircle( Vec3 position, float radius, Vec3 color ) {
    glUseProgram( rendererStorage.pShader.programID );

    Mat4 p; SetToIdentity( &p );
    SetTranslation( &p, position.x, position.y, position.z ); SetScale( &p, radius, radius, radius );
    glUniformMatrix4fv( rendererStorage.pShader.modelMatrixUniformPtr, 1, false, (float*)&p.m[0] );
    glUniformMatrix4fv( rendererStorage.pShader.cameraMatrixUniformPtr, 1, false, (float*)&rendererStorage.baseProjectionMatrix.m[0] );
    glUniform4f( glGetUniformLocation( rendererStorage.pShader.programID, "primitiveColor" ), color.x, color.y, color.z, 1.0f );

    glEnableVertexAttribArray( rendererStorage.pShader.positionAttribute );
    glBindBuffer( GL_ARRAY_BUFFER, rendererStorage.circleDataPtr );
    glVertexAttribPointer( rendererStorage.pShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, rendererStorage.circleIDataPtr );
    glDrawElements( GL_TRIANGLE_FAN, 18, GL_UNSIGNED_INT, NULL );
}

void RenderDebugLines( float* vertexData, uint8 dataCount, Mat4 transform, Vec3 color ) {
    glUseProgram( rendererStorage.pShader.programID );

    glUniformMatrix4fv( rendererStorage.pShader.modelMatrixUniformPtr, 1, false, (float*)&transform.m[0] );
    glUniformMatrix4fv( rendererStorage.pShader.cameraMatrixUniformPtr, 1, false, (float*)&rendererStorage.baseProjectionMatrix.m[0] );
    glUniform4f( glGetUniformLocation( rendererStorage.pShader.programID, "primitiveColor" ), color.x, color.y, color.z, 1.0f );

    glEnableVertexAttribArray( rendererStorage.pShader.positionAttribute );
    glBindBuffer( GL_ARRAY_BUFFER, rendererStorage.lineDataPtr );
    glBufferData( GL_ARRAY_BUFFER, dataCount * 3 * sizeof(float), (GLfloat*)vertexData, GL_DYNAMIC_DRAW );
    glVertexAttribPointer( rendererStorage.pShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, rendererStorage.lineIDataPtr );
    glDrawElements( GL_LINES, dataCount, GL_UNSIGNED_INT, NULL );
}

void RenderArmatureAsLines( Armature* armature, Mat4 transform, Vec3 color ) {
    uint8 dataCount = 0;
    Vec3 linePositions [64];

    for( uint8 boneIndex = 0; boneIndex < armature->boneCount; boneIndex++ ) {
        Bone* bone = &armature->allBones[ boneIndex ];
        Vec3 p1 = { 0.0f, 0.0f, 0.0f };
        p1 = MultVec( InverseMatrix( bone->invBindPose ), p1 );
        p1 = MultVec( *bone->currentTransform, p1 );

        RenderDebugCircle( MultVec( transform, p1 ), 0.05 );

        if( bone->childCount > 0 ) {
            for( uint8 childIndex = 0; childIndex < bone->childCount; childIndex++ ) {
                Bone* child = bone->children[ childIndex ];

                Vec3 p2 = { 0.0f, 0.0f, 0.0f };
                p2 = MultVec( InverseMatrix( child->invBindPose ), p2 );
                p2 = MultVec( *child->currentTransform, p2 );

                linePositions[ dataCount++ ] = p1;
                linePositions[ dataCount++ ] = p2;
            }
        } else {
            Vec3 p2 = { 0.0f, 1.5f, 0.0f };
            p2 = MultVec( InverseMatrix( bone->invBindPose ), p2 );
            p2 = MultVec( *bone->currentTransform, p2 );

            linePositions[ dataCount++ ] = p1;
            linePositions[ dataCount++ ] = p2;
        }
    }

    RenderDebugLines( (float*)&linePositions[0], dataCount, transform, color );
}