#define GL_FUNCS \
GLE( void, GenBuffers, GLsizei n, GLuint* buffers ) \
GLE( void, BindBuffer, GLenum target, GLuint buffer ) \
GLE( void, BufferData, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage ) \
\
GLE( boolean, isShader, GLuint shader ) \
GLE( void, GetShaderiv, GLuint shader, GLenum pname, GLint* params ) \
GLE( void, GetProgramiv, GLuint shader, GLenum pname, GLint* params ) \
\
GLE( GLuint, CreateShader, GLuint type ) \
GLE( void, ShaderSource, GLuint shader, GLsizei count, const GLchar** src, const GLint* length ) \
GLE( void, CompileShader, GLuint shader ) \
GLE( void, GetShaderInfoLog, GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog ) \
GLE( void, AttachShader, GLuint program, GLuint shader ) \
GLE( void, DeleteShader, GLuint shader ) \
GLE( void, DeleteProgram, GLuint program ) \
\
GLE( GLuint, CreateProgram, void ) \
GLE( void, LinkProgram, GLuint program ) \
GLE( void, UseProgram, GLuint program ) \
\
GLE( void, GetActiveAttrib, GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name ) \
GLE( void, EnableVertexAttribArray, GLuint index ) \
GLE( void, VertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer ) \
GLE( GLint, GetAttribLocation, GLuint program, const GLchar* name ) \
\
GLE( void, GetActiveUniform, GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name ) \
GLE( GLint, GetUniformLocation, GLuint program, const GLchar* name ) \
GLE( void, Uniform1i, GLint location, GLint value ) \
GLE( void, Uniform2fv, GLint location, GLsizei count, GLfloat* value ) \
GLE( void, Uniform3fv, GLint location, GLsizei count, GLfloat* value ) \
GLE( void, Uniform4fv, GLint location, GLsizei count, GLfloat* value ) \
GLE( void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLvoid* value ) \
\
GLE( void, ActiveTexture, GLenum texture ) \
\
GLE( GLenum, CheckFramebufferStatus, GLenum target ) \
\
GLE( void, GenFramebuffers, GLsizei n, GLuint* ids ) \
GLE( void, GenRenderbuffers, GLsizei n, GLuint* ids ) \
GLE( void, BindFramebuffer, GLenum target, GLuint framebuffer ) \
GLE( void, BindRenderbuffer, GLenum target, GLuint framebuffer ) \
GLE( void, RenderbufferStorage, GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) \
GLE( void, DrawBuffers, GLsizei n, GLenum* bufs ) \
GLE( void, FramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) \
GLE( void, FramebufferTexture, GLenum target, GLenum attachment, GLuint texture, GLint level ) \
GLE( void, FramebufferTexture2D, GLenum target, GLenum attachment, GLenum texTarget, GLuint texture, GLint level )

#define GLDECL WINAPI

#define GLE( ret, name, ... ) \
typedef ret GLDECL name##proc(__VA_ARGS__); \
static name##proc * gl##name = NULL;

GL_FUNCS

#undef GLE

struct GLRenderDriver {
    RenderDriver baseDriver;
    Framebuffer defaultFramebuffer;
};

/*----------------------------------------------------------------------------------
                                  Shader stuff
----------------------------------------------------------------------------------*/

static void PrintGLShaderLog( GLuint shader ) {
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
}

/*----------------------------------------------------------------------------------
                                 Texture stuff
-----------------------------------------------------------------------------------*/

static void CopyTextureDataToGpuMem( 
    TextureData* texData, 
    PtrToGpuMem* texBindID 
) {
    GLenum pixelFormat;
    if( texData->channelsPerPixel == 3 ) {
        pixelFormat = GL_RGB;
    } else if( texData->channelsPerPixel == 4 ) {
        pixelFormat = GL_RGBA;
    }

    GLuint glTextureID;
    glGenTextures( 1, &glTextureID );
    glBindTexture( GL_TEXTURE_2D, glTextureID );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );

    glTexImage2D( 
        GL_TEXTURE_2D, 
        0, 
        texData->channelsPerPixel, 
        texData->width, 
        texData->height, 
        0, 
        pixelFormat, 
        GL_UNSIGNED_BYTE, 
        texData->data 
    );

    *texBindID = glTextureID;

    //stbi_image_free( data );
}

static void ClearShaderProgram(
    ShaderProgram* shader
) {
    glDeleteProgram( shader->programID );
    memset( shader, 0, sizeof( ShaderProgram ) );
}

static void CreateShaderProgram(
    char* vertSrc, 
    char* fragSrc, 
    ShaderProgram* bindDataStorage 
) {
    bindDataStorage->programID = glCreateProgram();

    GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( vertexShader, 1, (const char**)&vertSrc, NULL );
    glCompileShader( vertexShader );

    GLint compiled = GL_FALSE;
    glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Could not compile Vertex Shader.\n" );
        PrintGLShaderLog( vertexShader );
    } else {
        printf( "Vertex Shader compiled.\n" );
        glAttachShader( bindDataStorage->programID, vertexShader );
    }

    GLuint fragShader = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( fragShader, 1, (const char**)&fragSrc, NULL );
    glCompileShader( fragShader );

    compiled = GL_FALSE;
    glGetShaderiv( fragShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Unable to compile fragment shader.\n");
        PrintGLShaderLog( fragShader );
    } else {
        printf( "Fragment Shader compiled. \n" );
        glAttachShader( bindDataStorage->programID, fragShader );
    }

    glLinkProgram( bindDataStorage->programID );
    //Check for errors
    compiled = GL_TRUE;
    glGetProgramiv( bindDataStorage->programID, GL_LINK_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Error linking program\n" );
        return;
    }
    printf( "Shader Program Linked Successfully\n");

    glUseProgram( bindDataStorage->programID );
    glDeleteShader( vertexShader );
    glDeleteShader( fragShader );

    uint32 nameWriteTargetOffset = 0;
    GLsizei nameLen;
    GLint attribSize;
    GLenum attribType;

    //Record All vertex inputs
    bindDataStorage->vertInputCount = 0;
    GLint activeGLAttributeCount;
    glGetProgramiv( bindDataStorage->programID, GL_ACTIVE_ATTRIBUTES, &activeGLAttributeCount );
    for( GLuint attributeIndex = 0; attributeIndex < activeGLAttributeCount; ++attributeIndex ) {
        char* nameWriteTarget = &bindDataStorage->nameBuffer[ nameWriteTargetOffset ];
        glGetActiveAttrib( bindDataStorage->programID, attributeIndex, 512 - nameWriteTargetOffset, &nameLen, &attribSize, &attribType, nameWriteTarget );
        bindDataStorage->vertexInputPtrs[ attributeIndex ] = glGetAttribLocation( bindDataStorage->programID, nameWriteTarget );
        bindDataStorage->vertexInputNames[ attributeIndex ] = nameWriteTarget;
        bindDataStorage->vertexInputTypes[ attributeIndex ] = attribType;
        nameWriteTargetOffset += nameLen + 1;
        bindDataStorage->vertInputCount++;
    }

    //Record all uniform info
    bindDataStorage->uniformCount = 0;
    bindDataStorage->samplerCount = 0;
    GLint activeGLUniformCount;
    glGetProgramiv( bindDataStorage->programID, GL_ACTIVE_UNIFORMS, &activeGLUniformCount );
    for( GLuint uniformIndex = 0; uniformIndex < activeGLUniformCount; ++uniformIndex ) {
        char* nameWriteTarget = &bindDataStorage->nameBuffer[ nameWriteTargetOffset ];
        glGetActiveUniform( bindDataStorage->programID, uniformIndex, 512 - nameWriteTargetOffset, &nameLen, &attribSize, &attribType, nameWriteTarget );
        if( attribType == GL_SAMPLER_2D ) {
            bindDataStorage->samplerPtrs[ bindDataStorage->samplerCount ] = glGetUniformLocation( bindDataStorage->programID, nameWriteTarget );
            bindDataStorage->samplerNames[ bindDataStorage->samplerCount++ ] = nameWriteTarget;
        } else {
            bindDataStorage->uniformPtrs[ uniformIndex - bindDataStorage->samplerCount ] = glGetUniformLocation( bindDataStorage->programID, nameWriteTarget );
            bindDataStorage->uniformNames[ uniformIndex - bindDataStorage->samplerCount ] = nameWriteTarget;
            bindDataStorage->uniformTypes[ uniformIndex - bindDataStorage->samplerCount ] = attribType;
            ++bindDataStorage->uniformCount;
        }
        nameWriteTargetOffset += nameLen + 1;       
     }

    glUseProgram(0);
}

static PtrToGpuMem AllocNewGpuArray() {
    PtrToGpuMem newBuff = 0;
    glGenBuffers( 1, &newBuff );

    return newBuff;
}

static void CopyDataToGpuArray( PtrToGpuMem copyTarget, void* data, uint64 dataSize ) {
    glBindBuffer( GL_ARRAY_BUFFER, copyTarget );
    glBufferData( GL_ARRAY_BUFFER, dataSize, data, GL_DYNAMIC_DRAW );
}

static void Draw( 
    RenderCommand* command, 
    bool doLines, 
    bool noBackFaceCull, 
    bool suppressDepthWrite,
    bool isWireFrame,
    bool dontDepthCheck 
) {
    //Bind Shader
    glUseProgram( command->shader->programID );

    //Vertex input data
    if( command->vertexFormat == RenderCommand::INTERLEAVESTREAM ) {
        glBindBuffer( GL_ARRAY_BUFFER, command->VertexFormat.bufferForStream );
        glBufferData( GL_ARRAY_BUFFER, command->VertexFormat.streamSize, command->VertexFormat.streamData, GL_DYNAMIC_DRAW );

        for( 
            int vertexInputIndex = 0;
            vertexInputIndex < command->shader->vertInputCount;
            ++vertexInputIndex
        ) {
            GLuint attribPtr = command->shader->vertexInputPtrs[ vertexInputIndex ];
            GLenum type = command->shader->vertexInputTypes[ vertexInputIndex ];

            int count;
            GLenum attType = GL_FLOAT;
            GLboolean normalize = GL_FALSE;
            if( type == GL_FLOAT_VEC4 ) {
                attType = GL_UNSIGNED_BYTE;
                normalize = GL_TRUE;
                count = 4;
            } else if( type == GL_FLOAT_VEC3 ) {
                count = 3;
            } else if( type == GL_FLOAT_VEC2 ) {
                count = 2;
            } else {
                //Oops! an unsupported vertex type!
                assert(false);
            }

            glEnableVertexAttribArray( attribPtr );
            glVertexAttribPointer( 
                attribPtr,
                count, 
                attType,
                normalize,
                command->VertexFormat.vertSize,
                (GLvoid*)command->VertexFormat.vertexAttributeOffsets[ vertexInputIndex ]
            );
        }
    } else if( command->vertexFormat == RenderCommand::SEPARATE_GPU_ARRAYS ) {
        for( 
            int attributeIndex = 0; 
            attributeIndex < command->shader->vertInputCount; 
            ++attributeIndex 
            ) {
            GLuint attribPtr = command->shader->vertexInputPtrs[ attributeIndex ];
            GLenum type = command->shader->vertexInputTypes[ attributeIndex ];

            uint32 attribBufferPtr = command->vertexInputData[ attributeIndex ];

            if( attribBufferPtr == 0 ) {
                continue;
            }

            int count;
            if( type == GL_FLOAT_VEC3 ) {
                count = 3;
            } else if( type == GL_FLOAT_VEC2 ) {
                count = 2;
            } else {
                //Oops! an unsupported vertex type!
                assert(false);
            }

            glBindBuffer( GL_ARRAY_BUFFER, attribBufferPtr );
            glEnableVertexAttribArray( attribPtr );
            glVertexAttribPointer( attribPtr, count, GL_FLOAT, GL_FALSE, 0, 0 );
        }
    }

    for( 
        int uniformIndex = 0; 
        uniformIndex < command->shader->uniformCount; 
        ++uniformIndex 
    ) {
        GLuint uniformPtr = command->shader->uniformPtrs[ uniformIndex ];
        GLenum type = command->shader->uniformTypes[ uniformIndex ];
        void* uniformData = command->uniformData[ uniformIndex ];

        if( uniformData == 0 ) {
            continue;
        }

        if( type == GL_FLOAT_VEC4 ) {
            glUniform4fv( uniformPtr, 1, (float*)uniformData );
        } else if( type == GL_FLOAT_MAT4 ) {
            glUniformMatrix4fv( uniformPtr, 1, GL_FALSE, (float*)uniformData );
        } else if( type == GL_FLOAT_VEC2 ) {
            glUniform2fv( uniformPtr, 1, (float*)uniformData );
        } else if( type == GL_FLOAT_VEC3 ) {
            glUniform3fv( uniformPtr, 1, (float*)uniformData );
        } else {
            //Oops! an unsupported uniform type!
            assert(false);
        }
    }

    for( 
        int samplerIndex = 0; 
        samplerIndex < command->shader->samplerCount; 
        ++samplerIndex 
    ) {
        if( command->samplerData == 0 ) {
            continue;
        }
        glActiveTexture( GL_TEXTURE0 + samplerIndex );
        glBindTexture( GL_TEXTURE_2D, command->samplerData[ samplerIndex ] );
        glUniform1i( command->shader->samplerPtrs[ samplerIndex ], samplerIndex );
    }

    GLenum primType = GL_TRIANGLES;
    if( doLines ) {
        primType = GL_LINE_STRIP;
    } else if ( isWireFrame ) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    if( noBackFaceCull ) {
        glDisable( GL_CULL_FACE );
    }
    if( suppressDepthWrite ) {
        glDepthMask( false );
    }
    if( dontDepthCheck ) {
        glDisable( GL_DEPTH_TEST );
    }

    glDrawArrays( primType, 0, command->elementCount );

    if( isWireFrame ) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }
    if( noBackFaceCull ) {
        glEnable( GL_CULL_FACE );
    }
    if( suppressDepthWrite ) {
        glDepthMask( true );
    }
    if( dontDepthCheck ) {
        glEnable( GL_DEPTH_TEST );
    }

    for( 
        int samplerIndex = 0; 
        samplerIndex < command->shader->samplerCount; 
        ++samplerIndex 
    ) {
        glActiveTexture( GL_TEXTURE0 + samplerIndex );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
    glUseProgram( 0 );
}

#if 0
struct TagInfo {
    char* start;
    int length;
}

static bool ParseColladaData( void* data, int32 dataLength ) {
    TagInfo libG_Data = FindFirstTag( "library_geometries", data, dataLength );

    //MeshGeometryData firstMesh = ReadFirstMeshData( geometriesTag );
    MeshGeometryData firstMesh = { };
    {
        int geometryTagLength = 0
        TagInfo geometryData = FindFirstTag( "geometry", libG_Data );

        TagInfo srcTag = FindFirstTag( "source", libG_Data );
        while( srcTag.start != NULL ) {
            //TODO: parse
            srcTag = FindFirstTag( "source", libG_Data );
        }

        TagInfo polylistTag = FindFirstTag( "polylist", libG_Data );
    }
}

#endif

#define TexturedQuadVertShaderSrc \
"#version 140\n" \
"uniform mat4 modelMatrix;" \
"attribute vec3 position;" \
"attribute vec2 texCoord;" \
"void main() { " \
"    gl_Position = modelMatrix * vec4( position, 1.0f);" \
"    gl_TexCoord[0] = vec4(texCoord.xy, 0.0f, 0.0f);" \
"}"

#define TexturedQuadFragShaderSrc \
"#version 140\n" \
"uniform sampler2D tex1;" \
"void main() {" \
"    gl_FragColor = texture( tex1, gl_TexCoord[0].xy );" \
"}"

static void CreateFramebuffer( 
    Framebuffer* framebuffer, 
    uint16 height, 
    uint16 width 
) {
    GLuint framebufferPtr;
    glGenFramebuffers( 1, &framebufferPtr );
    glBindFramebuffer( GL_FRAMEBUFFER, framebufferPtr );

    GLuint colorbufferTexturePtr;
    glGenTextures( 1, &colorbufferTexturePtr );
    glBindTexture( GL_TEXTURE_2D, colorbufferTexturePtr );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    GLuint depthbufferTexturePtr;
    glGenTextures( 1, &depthbufferTexturePtr );
    glBindTexture( GL_TEXTURE_2D, depthbufferTexturePtr );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );         

    glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorbufferTexturePtr, 0 );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthbufferTexturePtr, 0 );

    GLenum drawbuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers( 1, &drawbuffer );

    if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
        printf( "Failed to create new framebuffer\n" );
    }

    framebuffer->framebufferPtr = framebufferPtr;
    framebuffer->colorTexture = colorbufferTexturePtr;
    framebuffer->depthTexture = depthbufferTexturePtr;
    framebuffer->height = height;
    framebuffer->width = width;

    //reset to default framebuffer
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

static void SetFramebuffer( Framebuffer* framebuffer ) {
    glBindFramebuffer( GL_FRAMEBUFFER, framebuffer->framebufferPtr );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebuffer->colorTexture, 0 );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, framebuffer->depthTexture, 0 );

    GLenum drawbuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers( 1, &drawbuffer );

    if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
        printf( "Failed to set framebuffer\n" );
    }

    glViewport( 0, 0, framebuffer->width, framebuffer->height );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

static void ClearFramebuffer( System* system ) {
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glViewport( 0, 0, system->windowWidth, system->windowHeight );
}

static GLRenderDriver InitGLRenderer( 
    uint16 screen_w, 
    uint16 screen_h 
) {
    printf( "Vendor: %s\n", glGetString( GL_VENDOR ) );
    printf( "Renderer: %s\n", glGetString( GL_RENDERER ) );
    printf( "GL Version: %s\n", glGetString( GL_VERSION ) );
    printf( "GLSL Version: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

    GLint k;
    glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &k );
    printf( "Max texture units: %d\n", k );

    glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &k );
    printf( "Max Vertex Attributes: %d\n", k );

    //Initialize clear color
    glClearColor( 128.0f / 255.0f, 128.0f / 255.0f, 155.0f / 255.0f, 1.0f );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glCullFace( GL_BACK );
    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );

    glViewport( 0, 0, screen_w, screen_h );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR ) {
        printf( "Error initializing OpenGL\n" );
        GLRenderDriver driver = { };
        return driver;
    } else {
        printf( "Initialized OpenGL\n" );
    }

    GLRenderDriver driver = { };
    driver.baseDriver.ParseMeshDataFromCollada = &ParseMeshDataFromCollada;

    driver.baseDriver.AllocNewGpuArray = &AllocNewGpuArray;
    driver.baseDriver.CopyDataToGpuArray = &CopyDataToGpuArray;

    driver.baseDriver.CopyTextureDataToGpuMem = &CopyTextureDataToGpuMem;

    driver.baseDriver.CreateShaderProgram = &CreateShaderProgram;
    driver.baseDriver.ClearShaderProgram = &ClearShaderProgram;
    driver.baseDriver.Draw = &Draw;

    driver.baseDriver.CreateFramebuffer = &CreateFramebuffer;
    driver.baseDriver.SetFramebuffer = &SetFramebuffer;
    driver.baseDriver.ClearFramebuffer = &ClearFramebuffer;

    return driver;
}