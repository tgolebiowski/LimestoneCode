bool InitRenderer( uint16 screen_w, uint16 screen_h ) {
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
    //InitOrthoCameraMatrix( &baseOrthoMat, 20.0f * screenAspectRatio, 20.0f, -20.0f, 20.0f );
    //InitOrthoCameraMatrix( &baseOrthoMat, screen_w, screen_h, -1.0f, 1.0f );
    //cameraPosition = {0.0f, 0.0f, 0.0f};

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

    return true;
}

void CreateTextureBinding( TextureBindingID* texBindID, TextureDataStorage* textureData ) {
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

void CreateRenderBinding( MeshGPUBinding* bindDataStorage, MeshGeometryStorage* meshDataStorage, MeshSkinningStorage* meshSkinningStorage ) {
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

	bindDataStorage->dataCount = meshDataStorage->dataCount;
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
    //bindDataStorage->isArmatureAnimatedUniform = glGetUniformLocation( bindDataStorage->programID, "isSkeletalAnimated" );
    //bindDataStorage->skeletonUniform = glGetUniformLocation( bindDataStorage->programID, "skeleton" );

    bindDataStorage->positionAttribute = glGetAttribLocation( bindDataStorage->programID, "position" );
    bindDataStorage->texCoordAttribute = glGetAttribLocation( bindDataStorage->programID, "texCoord" );
    //bindDataStorage->boneWeightsAttribute = glGetAttribLocation( bindDataStorage->programID, "boneWeights" );
    //bindDataStorage->boneIndiciesAttribute = glGetAttribLocation( bindDataStorage->programID, "boneIndicies" );

    bindDataStorage->samplerPtr1 = glGetUniformLocation( bindDataStorage->programID, "tex1" );
    bindDataStorage->samplerPtr2 = glGetUniformLocation( bindDataStorage->programID, "tex2" );
    glUniform1i( bindDataStorage->samplerPtr1, 0 );
    glUniform1i( bindDataStorage->samplerPtr2, 1 );

    glUseProgram(0);
    glDeleteShader( vertexShader ); 
    glDeleteShader( fragShader );
}

void RenderBoundData( MeshGPUBinding* meshBinding, ShaderProgramBinding* programBinding, ShaderProgramParams params ) {
	//Flush errors
    //while( glGetError() != GL_NO_ERROR ){};

    //Bind Shader
    glUseProgram( programBinding->programID );
    glUniformMatrix4fv( programBinding->modelMatrixUniformPtr, 1, false, (float*)params.modelMatrix->m[0] );
    glUniformMatrix4fv( programBinding->cameraMatrixUniformPtr, 1, false, (float*)params.cameraMatrix->m[0] );
    //glUniform1i( program->isArmatureAnimatedUniform, (int)mesh->isArmatureAnimated );

    //Set vertex data
    glBindBuffer( GL_ARRAY_BUFFER, meshBinding->vertexDataPtr );
    glEnableVertexAttribArray( programBinding->positionAttribute );
    glVertexAttribPointer( programBinding->positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set UV data
    glBindBuffer( GL_ARRAY_BUFFER, meshBinding->uvDataPtr );
    glEnableVertexAttribArray( programBinding->texCoordAttribute );
    glVertexAttribPointer( programBinding->texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    // if( mesh->isArmatureAnimated ) {
    //     glUniformMatrix4fv( program->skeletonUniform, MAXBONECOUNT, GL_FALSE, (float*)&mesh->skeleton->boneTransforms->m[0] );

    //     glBindBuffer( GL_ARRAY_BUFFER, mesh->boneWeightBuffer );
    //     glEnableVertexAttribArray( program->boneWeightsAttribute );
    //     glVertexAttribPointer( program->boneWeightsAttribute, MAXBONEPERVERT, GL_FLOAT, false, 0, 0 );

    //     glBindBuffer( GL_ARRAY_BUFFER, mesh->boneIndexBuffer );
    //     glEnableVertexAttribArray( program->boneIndiciesAttribute );
    //     glVertexAttribIPointer( program->boneIndiciesAttribute, MAXBONEPERVERT, GL_UNSIGNED_INT, 0, 0 );
    // }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, params.sampler1 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, params.sampler2 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, meshBinding->indexDataPtr );
    glDrawElements( GL_TRIANGLES, meshBinding->dataCount, GL_UNSIGNED_INT, NULL );     ///Render, assume its all triangles

    //Unbind textures
    //glBindTexture( GL_TEXTURE_2D, 0 );
    //glActiveTexture( GL_TEXTURE0 );
    //glBindTexture( GL_TEXTURE_2D, 0 );
    //clear shader
    //glUseProgram( (GLuint)NULL );
}