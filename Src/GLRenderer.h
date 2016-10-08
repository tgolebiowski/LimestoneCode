struct GL_API {
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;

    PFNGLBINDBUFFERPROC glBindBuffer = NULL;
    PFNGLBUFFERDATAPROC glBufferData = NULL;
    PFNGLGENBUFFERSPROC glGenBuffers = NULL;

    //PFNGLGENTEXTURESPROC glGenTextures = NULL;
    //PFNGLBINDTEXTUREPROC glBindTexture = NULL;
    //PFNGLTEXPARAMETERIPROC glTexParameteri = NULL;
    //PFNGLTEXIMAGE2DPROC glTexImage2D = NULL;
    PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;

    PFNGLISSHADERPROC glIsShader = NULL;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
    PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
    PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;

    PFNGLCREATESHADERPROC glCreateShader = NULL;
    PFNGLSHADERSOURCEPROC glShaderSource = NULL;
    PFNGLCOMPILESHADERPROC glCompileShader = NULL;
    PFNGLATTACHSHADERPROC glAttachShader = NULL;
    PFNGLDELETESHADERPROC glDeleteShader = NULL;
    PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;

    PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
    PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
    PFNGLUSEPROGRAMPROC glUseProgram = NULL;

    PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = NULL;
    PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib = NULL;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;

    PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform = NULL;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
    PFNGLUNIFORM1IPROC glUniform1i = NULL;
    PFNGLUNIFORM4FVPROC glUniform4fv = NULL;
    PFNGLUNIFORM3FVPROC glUniform3fv = NULL;
    PFNGLUNIFORM2FVPROC glUniform2fv = NULL;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
};

struct GLRenderDriver {
    RenderDriver baseDriver;

    GL_API* glApi;
};

/*----------------------------------------------------------------------------------
                                  Shader stuff
----------------------------------------------------------------------------------*/

static void PrintGLShaderLog( GL_API* glApi, GLuint shader ) {
    //Shader log length
    int infoLogLength = 0;
    int maxLength = infoLogLength;

    //Get info string length
    glApi->glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );

    //Allocate string
    GLchar infoLog[ 256 ];

    //Get info log
    glApi->glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
    if( infoLogLength > 0 ) {
        //Print Log
        printf( "%s\n", infoLog );
    }
}

/*----------------------------------------------------------------------------------
                                 Texture stuff
-----------------------------------------------------------------------------------*/

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb/stb_image.h"

static void CopyTextureDataToGpuMem( 
    RenderDriver* renderDriver, 
    TextureData* texData, 
    PtrToGpuMem* texBindID 
) {
    GLRenderDriver* glRenderDriver = (GLRenderDriver*)renderDriver;
    GL_API* glApi = glRenderDriver->glApi;

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
    RenderDriver* driver,
    ShaderProgram* shader
) {
    GL_API* glApi = ((GLRenderDriver*)driver)->glApi;

    glApi->glDeleteProgram( shader->programID );
    memset( shader, 0, sizeof( ShaderProgram ) );
}

static void CreateShaderProgram( 
    RenderDriver* driver,
    char* vertSrc, 
    char* fragSrc, 
    ShaderProgram* bindDataStorage 
) {
    GLRenderDriver* glDriver = (GLRenderDriver*)driver;
    GL_API* glApi = glDriver->glApi;

    bindDataStorage->programID = glApi->glCreateProgram();

    GLuint vertexShader = glApi->glCreateShader( GL_VERTEX_SHADER );
    glApi->glShaderSource( vertexShader, 1, &vertSrc, NULL );
    glApi->glCompileShader( vertexShader );

    GLint compiled = GL_FALSE;
    glApi->glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Could not compile Vertex Shader.\n" );
        PrintGLShaderLog( glApi, vertexShader );
    } else {
        printf( "Vertex Shader compiled.\n" );
        glApi->glAttachShader( bindDataStorage->programID, vertexShader );
    }

    GLuint fragShader = glApi->glCreateShader( GL_FRAGMENT_SHADER );
    glApi->glShaderSource( fragShader, 1, &fragSrc, NULL );
    glApi->glCompileShader( fragShader );

    compiled = GL_FALSE;
    glApi->glGetShaderiv( fragShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Unable to compile fragment shader.\n");
        PrintGLShaderLog( glApi, fragShader );
    } else {
        glApi->glAttachShader( bindDataStorage->programID, fragShader );
    }

    glApi->glLinkProgram( bindDataStorage->programID );
    //Check for errors
    compiled = GL_TRUE;
    glApi->glGetProgramiv( bindDataStorage->programID, GL_LINK_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Error linking program\n" );
        return;
    }
    printf( "Shader Program Linked Successfully\n");

    glApi->glUseProgram( bindDataStorage->programID );
    glApi->glDeleteShader( vertexShader );
    glApi->glDeleteShader( fragShader );

    uint32 nameWriteTargetOffset = 0;
    GLsizei nameLen;
    GLint attribSize;
    GLenum attribType;

    //Record All vertex inputs
    bindDataStorage->vertInputCount = 0;
    GLint activeGLAttributeCount;
    glApi->glGetProgramiv( bindDataStorage->programID, GL_ACTIVE_ATTRIBUTES, &activeGLAttributeCount );
    for( GLuint attributeIndex = 0; attributeIndex < activeGLAttributeCount; ++attributeIndex ) {
        char* nameWriteTarget = &bindDataStorage->nameBuffer[ nameWriteTargetOffset ];
        glApi->glGetActiveAttrib( bindDataStorage->programID, attributeIndex, 512 - nameWriteTargetOffset, &nameLen, &attribSize, &attribType, nameWriteTarget );
        bindDataStorage->vertexInputPtrs[ attributeIndex ] = glApi->glGetAttribLocation( bindDataStorage->programID, nameWriteTarget );
        bindDataStorage->vertexInputNames[ attributeIndex ] = nameWriteTarget;
        bindDataStorage->vertexInputTypes[ attributeIndex ] = attribType;
        nameWriteTargetOffset += nameLen + 1;
        bindDataStorage->vertInputCount++;
    }

    //Record all uniform info
    bindDataStorage->uniformCount = 0;
    bindDataStorage->samplerCount = 0;
    GLint activeGLUniformCount;
    glApi->glGetProgramiv( bindDataStorage->programID, GL_ACTIVE_UNIFORMS, &activeGLUniformCount );
    for( GLuint uniformIndex = 0; uniformIndex < activeGLUniformCount; ++uniformIndex ) {
        char* nameWriteTarget = &bindDataStorage->nameBuffer[ nameWriteTargetOffset ];
        glApi->glGetActiveUniform( bindDataStorage->programID, uniformIndex, 512 - nameWriteTargetOffset, &nameLen, &attribSize, &attribType, nameWriteTarget );
        if( attribType == GL_SAMPLER_2D ) {
            bindDataStorage->samplerPtrs[ bindDataStorage->samplerCount ] = glApi->glGetUniformLocation( bindDataStorage->programID, nameWriteTarget );
            glApi->glUniform1i( bindDataStorage->samplerPtrs[ bindDataStorage->samplerCount ], bindDataStorage->samplerCount );
            bindDataStorage->samplerNames[ bindDataStorage->samplerCount++ ] = nameWriteTarget;
        } else {
            bindDataStorage->uniformPtrs[ uniformIndex - bindDataStorage->samplerCount ] = glApi->glGetUniformLocation( bindDataStorage->programID, nameWriteTarget );
            bindDataStorage->uniformNames[ uniformIndex - bindDataStorage->samplerCount ] = nameWriteTarget;
            bindDataStorage->uniformTypes[ uniformIndex - bindDataStorage->samplerCount ] = attribType;
            ++bindDataStorage->uniformCount;
        }
        nameWriteTargetOffset += nameLen + 1;       
     }

    glApi->glUseProgram(0);
}

static PtrToGpuMem CopyVertexDataToGpuMem( 
    RenderDriver* driver, 
    void* data, 
    size_t size 
) {
    GLRenderDriver* glDriver = (GLRenderDriver*)driver;
    GL_API* gl_api = glDriver->glApi;

    GLuint glVBOPtr;
    gl_api->glGenBuffers( 1, &glVBOPtr );
    gl_api->glBindBuffer( GL_ARRAY_BUFFER, glVBOPtr );
    gl_api->glBufferData( GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW );
    return glVBOPtr;
}

static PtrToGpuMem AllocGpuBuffForDynamicStream( RenderDriver* driver ) {
    GL_API* glApi = ((GLRenderDriver*)driver)->glApi;

    PtrToGpuMem newBuff = 0;
    glApi->glGenBuffers( 1, &newBuff );

    return newBuff;
}

static void DrawMesh( 
    RenderDriver* driver, 
    RenderCommand* command 
) {
    GLRenderDriver* glDriver = (GLRenderDriver*)driver;
    GL_API* gl_api = (GL_API*)glDriver->glApi;

    //Bind Shader
    gl_api->glUseProgram( command->shader->programID );

    //Vertex input data
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

        gl_api->glBindBuffer( GL_ARRAY_BUFFER, attribBufferPtr );
        gl_api->glEnableVertexAttribArray( attribPtr );
        gl_api->glVertexAttribPointer( attribPtr, count, GL_FLOAT, GL_FALSE, 0, 0 );
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
            gl_api->glUniform4fv( uniformPtr, 1, (float*)uniformData );
        } else if( type == GL_FLOAT_MAT4 ) {
            gl_api->glUniformMatrix4fv( uniformPtr, 1, GL_FALSE, (float*)uniformData );
        } else if( type == GL_FLOAT_VEC2 ) {
            gl_api->glUniform2fv( uniformPtr, 1, (float*)uniformData );
        } else if( type == GL_FLOAT_VEC3 ) {
            gl_api->glUniform3fv( uniformPtr, 1, (float*)uniformData );
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
        gl_api->glActiveTexture( GL_TEXTURE0 + samplerIndex );
        glBindTexture( GL_TEXTURE_2D, command->samplerData[ samplerIndex ] );
    }

    glDrawArrays( GL_TRIANGLES, 0, command->elementCount );

    for( 
        int samplerIndex = 0; 
        samplerIndex < command->shader->samplerCount; 
        ++samplerIndex 
    ) {
        gl_api->glActiveTexture( GL_TEXTURE0 + samplerIndex );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
    gl_api->glUseProgram( 0 );
}

static void DrawInterleavedStream(
    RenderDriver* driver,
    RenderCommand_Interleaved* command
) {
    GL_API* glApi = ((GLRenderDriver*)driver)->glApi;

    glApi->glBindBuffer( GL_ARRAY_BUFFER, command->bufferForStream );
    glApi->glBufferData( 
        GL_ARRAY_BUFFER,
        command->streamSize, 
        command->streamData, 
        GL_DYNAMIC_DRAW
    );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_SCISSOR_TEST );

    glApi->glUseProgram( command->shader->programID );

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

        glApi->glEnableVertexAttribArray( attribPtr );
        glApi->glVertexAttribPointer( 
            attribPtr,
            count, 
            attType,
            normalize,
            command->vertSize,
            (GLvoid*)command->vertexAttributeOffsets[ vertexInputIndex ]
        );
    }

    /*
    FOR REFERENCE, OLD IMGUI DRAW CODE
    glVertexAttribPointer( im_posAttribute, 2, GL_FLOAT, false, sizeof( ImDrawVert ), (GLvoid*)OFFSETOF( ImDrawVert, pos ) );
    glVertexAttribPointer( im_uvAttribute, 2, GL_FLOAT, true, sizeof( ImDrawVert ), (GLvoid*)OFFSETOF( ImDrawVert, uv ) );
    glVertexAttribPointer( im_colorAttribute, 4, GL_UNSIGNED_BYTE, true, sizeof( ImDrawVert ), (GLvoid*)OFFSETOF( ImDrawVert, col ) );
    */

    #if 0
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
            gl_api->glUniform4fv( uniformPtr, 1, (float*)uniformData );
        } else if( type == GL_FLOAT_MAT4 ) {
            gl_api->glUniformMatrix4fv( uniformPtr, 1, GL_FALSE, (float*)uniformData );
        } else if( type == GL_FLOAT_VEC2 ) {
            gl_api->glUniform2fv( uniformPtr, 1, (float*)uniformData );
        } else if( type == GL_FLOAT_VEC3 ) {
            gl_api->glUniform3fv( uniformPtr, 1, (float*)uniformData );
        } else {
            //Oops! an unsupported uniform type!
            assert(false);
        }
    }
    #endif

    for( 
        int samplerIndex = 0; 
        samplerIndex < command->shader->samplerCount; 
        ++samplerIndex 
    ) {
        if( command->sampleData[ samplerIndex ] == 0 ) {
            continue;
        }
        glApi->glActiveTexture( GL_TEXTURE0 + samplerIndex );
        glBindTexture( GL_TEXTURE_2D, command->sampleData[ samplerIndex ] );
    }

    glDrawArrays( GL_TRIANGLES, 0, command->elementCount );

    for( 
        int samplerIndex = 0; 
        samplerIndex < command->shader->samplerCount; 
        ++samplerIndex 
    ) {
        glApi->glActiveTexture( GL_TEXTURE0 + samplerIndex );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
    glApi->glUseProgram( 0 ); 
    glDisable( GL_BLEND );
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glDisable( GL_SCISSOR_TEST );
}

#if 0
static void RenderTexturedQuad( 
    void* api, 
    RendererStorage* rendererStorage, 
    PtrToGpuMem texture, 
    float width, float height, 
    float x, float y 
) {
    GL_API* gl_api = (GL_API*)api;

    Mat4 transform, translation, scale; 
    SetToIdentity( &translation ); 
    SetToIdentity( &scale );
    SetScale( &scale, width, height, 1.0f  ); 
    SetTranslation( &translation, x, y, 0.0f );
    transform = MultMatrix( scale, translation );

    gl_api->glUseProgram( rendererStorage->texturedQuadShader.programID );
    gl_api->glUniformMatrix4fv( rendererStorage->quadMat4UniformPtr, 1, false, (float*)&transform );

    //Set vertex data
    gl_api->glBindBuffer( GL_ARRAY_BUFFER, rendererStorage->quadVDataPtr );
    gl_api->glEnableVertexAttribArray( rendererStorage->quadPosAttribPtr );
    gl_api->glVertexAttribPointer( rendererStorage->quadPosAttribPtr, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set UV data
    gl_api->glBindBuffer( GL_ARRAY_BUFFER, rendererStorage->quadUVDataPtr );
    gl_api->glEnableVertexAttribArray( rendererStorage->quadUVAttribPtr );
    gl_api->glVertexAttribPointer( rendererStorage->quadUVAttribPtr, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    gl_api->glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, (GLuint)texture );

    gl_api->glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, rendererStorage->quadIDataPtr );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL );
}
#endif

#include "tinyxml2/tinyxml2.h"
#include "tinyxml2/tinyxml2.cpp"
static bool ParseMeshDataFromCollada( void* rawData, Stack* allocater, MeshGeometryData* storage, Armature* armature ) {
    tinyxml2::XMLDocument colladaDoc;
    colladaDoc.Parse( (const char*)rawData );

    tinyxml2::XMLElement* meshNode = colladaDoc.FirstChildElement( "COLLADA" )->FirstChildElement( "library_geometries" )
    ->FirstChildElement( "geometry" )->FirstChildElement( "mesh" );

    char* colladaTextBuffer = NULL;
    size_t textBufferLen = 0;

    uint16 vCount = 0;
    uint16 nCount = 0;
    uint16 uvCount = 0;
    uint16 indexCount = 0;
    float* rawColladaVertexData;
    float* rawColladaNormalData;
    float* rawColladaUVData;
    float* rawIndexData;
    ///Basic Mesh Geometry Data
    {
        tinyxml2::XMLNode* colladaVertexArray = meshNode->FirstChildElement( "source" );
        tinyxml2::XMLElement* vertexFloatArray = colladaVertexArray->FirstChildElement( "float_array" );
        tinyxml2::XMLNode* colladaNormalArray = colladaVertexArray->NextSibling();
        tinyxml2::XMLElement* normalFloatArray = colladaNormalArray->FirstChildElement( "float_array" );
        tinyxml2::XMLNode* colladaUVMapArray = colladaNormalArray->NextSibling();
        tinyxml2::XMLElement* uvMapFloatArray = colladaUVMapArray->FirstChildElement( "float_array" );
        tinyxml2::XMLElement* meshSrc = meshNode->FirstChildElement( "polylist" );
        tinyxml2::XMLElement* colladaIndexArray = meshSrc->FirstChildElement( "p" );

        int count;
        const char* colladaVertArrayVal = vertexFloatArray->FirstChild()->Value();
        vertexFloatArray->QueryAttribute( "count", &count );
        vCount = count;
        const char* colladaNormArrayVal = normalFloatArray->FirstChild()->Value();
        normalFloatArray->QueryAttribute( "count", &count );
        nCount = count;
        const char* colladaUVMapArrayVal = uvMapFloatArray->FirstChild()->Value();
        uvMapFloatArray->QueryAttribute( "count", &count );
        uvCount = count;
        const char* colladaIndexArrayVal = colladaIndexArray->FirstChild()->Value();
        meshSrc->QueryAttribute( "count", &count );
        //Assume this is already triangulated
        indexCount = count * 3 * 3;

        ///TODO: replace this with fmaxf?
        std::function< size_t (size_t, size_t) > sizeComparison = []( size_t size1, size_t size2 ) -> size_t {
            if( size1 >= size2 ) return size1;
            return size2;
        };

        textBufferLen = strlen( colladaVertArrayVal );
        textBufferLen = sizeComparison( strlen( colladaNormArrayVal ), textBufferLen );
        textBufferLen = sizeComparison( strlen( colladaUVMapArrayVal ), textBufferLen );
        textBufferLen = sizeComparison( strlen( colladaIndexArrayVal ), textBufferLen );
        colladaTextBuffer = (char*)alloca( textBufferLen );
        memset( colladaTextBuffer, 0, textBufferLen );
        rawColladaVertexData = (float*)alloca( sizeof(float) * vCount );
        rawColladaNormalData = (float*)alloca( sizeof(float) * nCount );
        rawColladaUVData = (float*)alloca( sizeof(float) * uvCount );
        rawIndexData = (float*)alloca( sizeof(float) * indexCount );

        memset( rawColladaVertexData, 0, sizeof(float) * vCount );
        memset( rawColladaNormalData, 0, sizeof(float) * nCount );
        memset( rawColladaUVData, 0, sizeof(float) * uvCount );
        memset( rawIndexData, 0, sizeof(float) * indexCount );

        //Reading Vertex position data
        strcpy( colladaTextBuffer, colladaVertArrayVal );
        TextToNumberConversion( colladaTextBuffer, rawColladaVertexData );

        //Reading Normals data
        memset( colladaTextBuffer, 0, textBufferLen );
        strcpy( colladaTextBuffer, colladaNormArrayVal );
        TextToNumberConversion( colladaTextBuffer, rawColladaNormalData );

        //Reading UV map data
        memset( colladaTextBuffer, 0, textBufferLen );
        strcpy( colladaTextBuffer, colladaUVMapArrayVal );
        TextToNumberConversion( colladaTextBuffer, rawColladaUVData );

        //Reading index data
        memset( colladaTextBuffer, 0, textBufferLen );
        strcpy( colladaTextBuffer, colladaIndexArrayVal );
        TextToNumberConversion( colladaTextBuffer, rawIndexData );
    }

    float* rawBoneWeightData = NULL;
    float* rawBoneIndexData = NULL;
    //Skinning Data
    {
        tinyxml2::XMLElement* libControllers = colladaDoc.FirstChildElement( "COLLADA" )->FirstChildElement( "library_controllers" );
        if( libControllers == NULL ) goto skinningExit;
        tinyxml2::XMLElement* controllerElement = libControllers->FirstChildElement( "controller" );
        if( controllerElement == NULL ) goto skinningExit;

        tinyxml2::XMLNode* vertexWeightDataArray = controllerElement->FirstChild()->FirstChild()->NextSibling()->NextSibling()->NextSibling();
        tinyxml2::XMLNode* vertexBoneIndexDataArray = vertexWeightDataArray->NextSibling()->NextSibling();
        tinyxml2::XMLNode* vCountArray = vertexBoneIndexDataArray->FirstChildElement( "vcount" );
        tinyxml2::XMLNode* vArray = vertexBoneIndexDataArray->FirstChildElement( "v" );
        const char* boneWeightsData = vertexWeightDataArray->FirstChild()->FirstChild()->Value();
        const char* vCountArrayData = vCountArray->FirstChild()->Value();
        const char* vArrayData = vArray->FirstChild()->Value();

        float* colladaBoneWeightData = NULL;
        float* colladaBoneIndexData = NULL;
        float* colladaBoneInfluenceCounts = NULL;
        ///This is overkill, Collada stores ways less data usually, plus this still doesn't account for very complex models 
        ///(e.g, lots of verts with more than MAXBONESPERVERT influencing position )
        colladaBoneWeightData = (float*)alloca( sizeof(float) * MAXBONESPERVERT * vCount );
        colladaBoneIndexData = (float*)alloca( sizeof(float) * MAXBONESPERVERT * vCount );
        colladaBoneInfluenceCounts = (float*)alloca( sizeof(float) * MAXBONESPERVERT * vCount );

        //Read bone weights data
        memset( colladaTextBuffer, 0, textBufferLen );
        strcpy( colladaTextBuffer, boneWeightsData );
        TextToNumberConversion( colladaTextBuffer, colladaBoneWeightData );

        //Read bone index data
        memset( colladaTextBuffer, 0, textBufferLen );
        strcpy( colladaTextBuffer, vArrayData );
        TextToNumberConversion( colladaTextBuffer, colladaBoneIndexData );

        //Read bone influence counts
        memset( colladaTextBuffer, 0, textBufferLen );
        strcpy( colladaTextBuffer, vCountArrayData );
        TextToNumberConversion( colladaTextBuffer, colladaBoneInfluenceCounts );

        rawBoneWeightData = (float*)alloca( sizeof(float) * MAXBONESPERVERT * vCount );
        rawBoneIndexData = (float*)alloca( sizeof(float) * MAXBONESPERVERT * vCount );
        memset( rawBoneWeightData, 0, sizeof(float) * MAXBONESPERVERT * vCount );
        memset( rawBoneIndexData, 0, sizeof(float) * MAXBONESPERVERT * vCount );

        int colladaIndexIndirection = 0;
        int verticiesInfluenced = 0;
        vCountArray->Parent()->ToElement()->QueryAttribute( "count", &verticiesInfluenced );
        for( uint16 i = 0; i < verticiesInfluenced; i++ ) {
            uint8 influenceCount = colladaBoneInfluenceCounts[i];
            for( uint16 j = 0; j < influenceCount; j++ ) {
                uint16 boneIndex = colladaBoneIndexData[ colladaIndexIndirection++ ];
                uint16 weightIndex = colladaBoneIndexData[ colladaIndexIndirection++ ];
                rawBoneWeightData[ i * MAXBONESPERVERT + j ] = colladaBoneWeightData[ weightIndex ];
                rawBoneIndexData[ i * MAXBONESPERVERT + j ] = boneIndex;
            }
        }
    }
    skinningExit:

    //Armature
    if( armature != NULL ) {
        tinyxml2::XMLElement* visualScenesNode = colladaDoc.FirstChildElement( "COLLADA" )->FirstChildElement( "library_visual_scenes" )
        ->FirstChildElement( "visual_scene" )->FirstChildElement( "node" );
        tinyxml2::XMLElement* armatureNode = NULL;

        //Step through scene heirarchy until start of armature is found
        while( visualScenesNode != NULL ) {
            if( visualScenesNode->FirstChildElement( "node" ) != NULL && 
                visualScenesNode->FirstChildElement( "node" )->Attribute( "type", "JOINT" ) != NULL ) {
                armatureNode = visualScenesNode;
                break;
            } else {
                visualScenesNode = visualScenesNode->NextSibling()->ToElement();
            }
        }
        if( armatureNode == NULL ) {
            printf( "Armature storage needed to load but not provided\n" );
            return false;
        }

        //Parsing basic bone data from XML
        std::function< Bone* ( tinyxml2::XMLElement*, Armature*, Bone*  ) > ParseColladaBoneData = 
        [&]( tinyxml2::XMLElement* boneElement, Armature* armature, Bone* parentBone ) -> Bone* {
            Bone* bone = &armature->bones[ armature->boneCount ];
            bone->parent = parentBone;
            bone->currentTransform = &armature->boneTransforms[ armature->boneCount ];
            SetToIdentity( bone->currentTransform );
            bone->boneIndex = armature->boneCount;
            armature->boneCount++;

            strcpy( &bone->name[0], boneElement->Attribute( "sid" ) );

            float matrixData[16];
            char matrixTextData [512];
            tinyxml2::XMLNode* matrixElement = boneElement->FirstChildElement("matrix");
            strcpy( &matrixTextData[0], matrixElement->FirstChild()->ToText()->Value() );
            TextToNumberConversion( matrixTextData, matrixData );
            //Note: this is only local transform data, but its being saved in bind matrix for now
            Mat4 m;
            memcpy( &m.m[0][0], &matrixData[0], sizeof(float) * 16 );
            bone->bindPose = TransposeMatrix( m );

            if( parentBone == NULL ) {
                armature->rootBone = bone;
            } else {
                bone->bindPose = MultMatrix( parentBone->bindPose, bone->bindPose );
            }

            bone->childCount = 0;
            tinyxml2::XMLElement* childBoneElement = boneElement->FirstChildElement( "node" );
            while( childBoneElement != NULL ) {
                Bone* childBone = ParseColladaBoneData( childBoneElement, armature, bone );
                bone->children[ bone->childCount++ ] = childBone;
                tinyxml2::XMLNode* siblingNode = childBoneElement->NextSibling();
                if( siblingNode != NULL ) {
                    childBoneElement = siblingNode->ToElement();
                } else {
                    childBoneElement = NULL;
                }
            };

            return bone;
        };
        armature->boneCount = 0;
        tinyxml2::XMLElement* boneElement = armatureNode->FirstChildElement( "node" );
        ParseColladaBoneData( boneElement, armature, NULL );

        //Parse inverse bind pose data from skinning section of XML
        {
            tinyxml2::XMLElement* boneNamesSource = colladaDoc.FirstChildElement( "COLLADA" )->FirstChildElement( "library_controllers" )
            ->FirstChildElement( "controller" )->FirstChildElement( "skin" )->FirstChildElement( "source" );
            tinyxml2::XMLElement* boneBindPoseSource = boneNamesSource->NextSibling()->ToElement();

            char* boneNamesLocalCopy = NULL;
            float* boneMatriciesData = (float*)alloca( sizeof(float) * 16 * armature->boneCount );
            const char* boneNameArrayData = boneNamesSource->FirstChild()->FirstChild()->Value();
            const char* boneMatrixTextData = boneBindPoseSource->FirstChild()->FirstChild()->Value();
            size_t nameDataLen = strlen( boneNameArrayData );
            size_t matrixDataLen = strlen( boneMatrixTextData );
            boneNamesLocalCopy = (char*)alloca( nameDataLen + 1 );
            memset( boneNamesLocalCopy, 0, nameDataLen + 1 );
            assert( textBufferLen > matrixDataLen );
            memcpy( boneNamesLocalCopy, boneNameArrayData, nameDataLen );
            memcpy( colladaTextBuffer, boneMatrixTextData, matrixDataLen );
            TextToNumberConversion( colladaTextBuffer, boneMatriciesData );
            char* nextBoneName = &boneNamesLocalCopy[0];
            for( uint8 matrixIndex = 0; matrixIndex < armature->boneCount; matrixIndex++ ) {
                Mat4 matrix;
                memcpy( &matrix.m[0], &boneMatriciesData[matrixIndex * 16], sizeof(float) * 16 );

                char boneName [32];
                char* boneNameEnd = nextBoneName;
                do {
                    boneNameEnd++;
                } while( *boneNameEnd != ' ' && *boneNameEnd != 0 );
                size_t charCount = boneNameEnd - nextBoneName;
                memset( boneName, 0, sizeof( char ) * 32 );
                memcpy( boneName, nextBoneName, charCount );
                nextBoneName = boneNameEnd + 1;

                Bone* targetBone = NULL;
                for( uint8 boneIndex = 0; boneIndex < armature->boneCount; boneIndex++ ) {
                    Bone* bone = &armature->bones[ boneIndex ];
                    if( strcmp( bone->name, boneName ) == 0 ) {
                        targetBone = bone;
                        break;
                    }
                }

                Mat4 correction;
                correction.m[0][0] = 1.0f; correction.m[0][1] = 0.0f; correction.m[0][2] = 0.0f; correction.m[0][3] = 0.0f;
                correction.m[1][0] = 0.0f; correction.m[1][1] = 0.0f; correction.m[1][2] = 1.0f; correction.m[1][3] = 0.0f;
                correction.m[2][0] = 0.0f; correction.m[2][1] = -1.0f; correction.m[2][2] = 0.0f; correction.m[2][3] = 0.0f;
                correction.m[3][0] = 0.0f; correction.m[3][1] = 0.0f; correction.m[3][2] = 0.0f; correction.m[3][3] = 1.0f;
                targetBone->invBindPose = TransposeMatrix( matrix );
                targetBone->invBindPose = MultMatrix( correction, targetBone->invBindPose );
            }
        }
    }

    //output to my version of storage
    storage->dataCount = 0;
    uint16 counter = 0;

    const uint32 vertCount = indexCount / 3;
    storage->vData = (Vec3*)StackAllocA( allocater, vertCount * sizeof( Vec3 ), 16 );
    storage->uvData = (float*)StackAllocA( allocater, vertCount * 2 * sizeof( float ), 4 );
    storage->normalData = (Vec3*)StackAllocA( allocater, vertCount * sizeof( Vec3 ), 4 );
    if( rawBoneWeightData != NULL ) {
        storage->boneWeightData = (float*)StackAllocA( allocater, sizeof(float) * vertCount * MAXBONESPERVERT, 4 );
        storage->boneIndexData = (uint32*)StackAllocA( allocater, sizeof(uint32) * vertCount * MAXBONESPERVERT, 4 );
    } else {
        storage->boneWeightData = NULL;
        storage->boneIndexData = NULL;
    }

    while( counter < indexCount ) {
        Vec3 v, n;
        float uv_x, uv_y;

        uint16 vertIndex = rawIndexData[ counter++ ];
        uint16 normalIndex = rawIndexData[ counter++ ];
        uint16 uvIndex = rawIndexData[ counter++ ];

        v.x = rawColladaVertexData[ vertIndex * 3 + 0 ];
        v.z = -rawColladaVertexData[ vertIndex * 3 + 1 ];
        v.y = rawColladaVertexData[ vertIndex * 3 + 2 ];

        n.x = rawColladaNormalData[ normalIndex * 3 + 0 ];
        n.z = -rawColladaNormalData[ normalIndex * 3 + 1 ];
        n.y = rawColladaNormalData[ normalIndex * 3 + 2 ];

        uv_x = rawColladaUVData[ uvIndex * 2 ];
        uv_y = rawColladaUVData[ uvIndex * 2 + 1 ];

        ///TODO: check for exact copies of data, use to index to first instance instead
        uint32 storageIndex = storage->dataCount;
        storage->vData[ storageIndex ] = v;
        storage->normalData[ storageIndex ] = n;
        storage->uvData[ storageIndex * 2 ] = uv_x;
        storage->uvData[ storageIndex * 2 + 1 ] = uv_y;
        if( rawBoneWeightData != NULL ) {
            uint16 boneDataIndex = storage->dataCount * MAXBONESPERVERT;
            uint16 boneVertexIndex = vertIndex * MAXBONESPERVERT;
            for( uint8 i = 0; i < MAXBONESPERVERT; i++ ) {
                storage->boneWeightData[ boneDataIndex + i ] = rawBoneWeightData[ boneVertexIndex + i ];
                storage->boneIndexData[ boneDataIndex + i ] = rawBoneIndexData[ boneVertexIndex + i ];
            }
        }
        storage->dataCount++;
    };

    return true;
}

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

static GLRenderDriver InitGLRenderer( 
    GL_API* glApi, 
    uint16 screen_w, 
    uint16 screen_h 
) {

    GLRenderDriver driver = { };
    driver.baseDriver.CopyVertexDataToGpuMem = &CopyVertexDataToGpuMem;
    driver.baseDriver.CreateShaderProgram = &CreateShaderProgram;
    driver.baseDriver.DrawMesh = &DrawMesh;
    driver.baseDriver.ParseMeshDataFromCollada = &ParseMeshDataFromCollada;
    driver.baseDriver.CopyTextureDataToGpuMem = &CopyTextureDataToGpuMem;
    driver.baseDriver.AllocGpuBuffForDynamicStream = &AllocGpuBuffForDynamicStream;
    driver.baseDriver.DrawInterleavedStream = &DrawInterleavedStream;
    driver.baseDriver.ClearShaderProgram = &ClearShaderProgram;

    driver.glApi = glApi;

	printf( "Vendor: %s\n", glGetString( GL_VENDOR ) );
    printf( "Renderer: %s\n", glGetString( GL_RENDERER ) );
    printf( "GL Version: %s\n", glGetString( GL_VERSION ) );
    printf( "GLSL Version: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

    glFrontFace( GL_CCW );

    GLint k;
    glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &k );
    printf( "Max texture units: %d\n", k );

    glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &k );
    printf( "Max Vertex Attributes: %d\n", k );

    //Initialize clear color
    glClearColor( 1.0f / 255.0f, 200.0f / 255.0f, 175.0f / 255.0f, 1.0f );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glViewport( 0, 0, screen_w, screen_h );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR ) {
        //printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        printf( "Error initializing OpenGL\n" );
        GLRenderDriver driver = { };
        return driver;
    } else {
        printf( "Initialized OpenGL\n" );
    }

    return driver;
}