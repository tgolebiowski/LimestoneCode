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
GLE( void, GenFrameBuffers, GLsizei n, GLuint* ids ) \
GLE( void, BindFramebuffer, GLenum target, GLuint framebuffer ) \
GLE( void, FramebufferTexture2D, GLenum target, GLenum attachment, GLenum texTarget, GLuint texture, GLint level )

#define GLDECL WINAPI

#define GLE( ret, name, ... ) \
typedef ret GLDECL name##proc(__VA_ARGS__); \
static name##proc * gl##name = NULL;

GL_FUNCS

#undef GLE

struct GLRenderDriver {
    RenderDriver baseDriver;
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

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb/stb_image.h"

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

static PtrToGpuMem CopyVertexDataToGpuMem( 
    void* data, 
    size_t size 
) {
    GLuint glVBOPtr;
    glGenBuffers( 1, &glVBOPtr );
    glBindBuffer( GL_ARRAY_BUFFER, glVBOPtr );
    glBufferData( GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW );
    return glVBOPtr;
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

static void Draw( RenderCommand* command, bool doLines, bool noBackFaceCull ) {
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
    } else if( command->vertexFormat == RenderCommand::SEPARATE_GPU_BUFFS ) {
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
    }

    GLenum primType = GL_TRIANGLES;
    if( doLines ) {
        primType = GL_LINES;
    }

    if( noBackFaceCull ) {
        glDisable( GL_CULL_FACE );
    }

    glDrawArrays( primType, 0, command->elementCount );

    if( noBackFaceCull ) {
        glEnable( GL_CULL_FACE );
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
    storage->vData = (Vec3*)StackAllocAligned( allocater, vertCount * sizeof( Vec3 ), 16 );
    storage->uvData = (float*)StackAllocAligned( allocater, vertCount * 2 * sizeof( float ), 4 );
    storage->normalData = (Vec3*)StackAllocAligned( allocater, vertCount * sizeof( Vec3 ), 4 );
    if( rawBoneWeightData != NULL ) {
        storage->boneWeightData = (float*)StackAllocAligned( allocater, sizeof(float) * vertCount * MAXBONESPERVERT, 4 );
        storage->boneIndexData = (uint32*)StackAllocAligned( allocater, sizeof(uint32) * vertCount * MAXBONESPERVERT, 4 );
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
    glClearColor( 128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f );

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
        //printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        printf( "Error initializing OpenGL\n" );
        GLRenderDriver driver = { };
        return driver;
    } else {
        printf( "Initialized OpenGL\n" );
    }

    GLRenderDriver driver = { };
    driver.baseDriver.ParseMeshDataFromCollada = &ParseMeshDataFromCollada;
    driver.baseDriver.AllocNewGpuArray = &AllocNewGpuArray;
    driver.baseDriver.CopyVertexDataToGpuMem = &CopyVertexDataToGpuMem;
    driver.baseDriver.CopyDataToGpuArray = &CopyDataToGpuArray;
    driver.baseDriver.CopyTextureDataToGpuMem = &CopyTextureDataToGpuMem;
    driver.baseDriver.CreateShaderProgram = &CreateShaderProgram;
    driver.baseDriver.ClearShaderProgram = &ClearShaderProgram;
    driver.baseDriver.Draw = &Draw;

    return driver;
}