
#include "assimp\cimport.h"               // Assimp Plain-C interface
#include "assimp\scene.h"                 // Assimp Output data structure
#include "assimp\postprocess.h"           // Assimp Post processing flags

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb\stb_image.h"

struct OpenGLTexture {
    GLuint textureID;
    GLenum pixelFormat;
    int width;
    int height;
    GLuint* data;
};

struct ShaderProgram {
    GLuint programID;

    GLint modelMatrixUniformPtr;

    GLint positionAttribute;
    GLint texCoordAttribute;

    GLint isArmatureAnimatedUniform;
    GLint skeletonUniform;
    GLint boneWeightsAttribute;
    GLint boneIndiciesAttribute;

    uint8_t samplerCount;
    GLint samplerPtrs[4];
};

struct ShaderTextureSet {
    uint8_t count;
    ShaderProgram* associatedShader;
    GLuint shaderSamplerPtrs[4];
    GLuint texturePtrs[4];
};

struct OpenGLMeshBinding {
	GLuint vertexCount;
	GLuint vbo;
	GLuint ibo;
    GLuint uvBuffer;

    Matrix4* modelMatrix;

    bool isArmatureAnimated;
    Skeleton* skeleton;
    GLuint boneWeightBuffer;
    GLuint boneIndexBuffer;
};

struct OpenGLFramebuffer {
    GLuint framebufferPtr;
    OpenGLTexture framebufferTexture;
    GLuint framebuffVBO;
    GLuint framebuffIBO;
    GLuint framebuffUVBuff;
};
static OpenGLFramebuffer myFramebuffer;
static ShaderProgram framebufferShader;

struct GameMemory {
    ShaderProgram lineShader;
    float vertexData[9];
    uint32 indexData[3];
    float x, y;
};

static ShaderProgram primitiveShader;

//THIS IS FOR DEBUGGING PURPOSES ONLY, DON'T SHIP WITH THIS, BAD PROGRAMMING PRACTICE
void RenderLineStrip( GLfloat* stripVertBuffer, uint32* indexBuffer, uint16 bufferCount ) {
    glUseProgram( primitiveShader.programID );

    const float identityM[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    glUniformMatrix4fv( primitiveShader.modelMatrixUniformPtr, 1, false, (float*)&identityM );

    GLuint lineVBO;
    glGenBuffers( 1, &lineVBO );
    glBindBuffer( GL_ARRAY_BUFFER, lineVBO );
    glBufferData( GL_ARRAY_BUFFER, 3 * bufferCount * sizeof(GLfloat), stripVertBuffer, GL_STATIC_DRAW );
    glEnableVertexAttribArray( primitiveShader.positionAttribute );
    glVertexAttribPointer( primitiveShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint lineIBO;
    glGenBuffers( 1, &lineIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, lineIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, bufferCount * sizeof(GLuint), (GLuint*)indexBuffer, GL_STATIC_DRAW );

    glDrawElements( GL_LINE_STRIP, bufferCount, GL_UNSIGNED_INT, NULL );

    glDeleteBuffers( 1, &lineVBO );
    glDeleteBuffers( 1, &lineIBO );
    glUseProgram( (GLuint)NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

//THIS IS FOR DEBUGGING PURPOSES ONLY, DON'T SHIP WITH THIS, BAD PROGRAMMING PRACTICE
void RenderCircle( Vec3 position, float radius ) {
    const GLfloat circlePoints[13 * 3] = { 0.0f, 0.0f, 0.0f, 
                                    1.0f, 0.0f, 0.0f, 
                                    0.866f, 0.5f, 0.0f,
                                    0.5f, 0.866f, 0.0f,
                                    0.0f, 1.0f, 0.0f,
                                    -0.5f, 0.866f, 0.0f,
                                    -0.866f, 0.5f, 0.0f,
                                    -1.0f, 0.0f, 0.0f,
                                    -0.866f, -0.5f, 0.0f,
                                    -0.5f, -0.866f, 0.0f,
                                    0.0f, -1.0f, 0.0f,
                                    0.5f, -0.866f, 0.0f,
                                    0.866f, -0.5f, 0.0f };

    const GLuint circleIndicies[14] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1 };

    glUseProgram( primitiveShader.programID );

    Matrix4 translateMatrix, scaleMatrix, netMatrix;
    SetTranslation( &translateMatrix, position.x, position.y, position.z );
    SetScale( &scaleMatrix, radius, radius, radius );
    netMatrix = scaleMatrix * translateMatrix;
    glUniformMatrix4fv( primitiveShader.modelMatrixUniformPtr, 1, false, (float*)&netMatrix.m[0] );

    GLuint circleVBO;
    glGenBuffers( 1, &circleVBO );
    glBindBuffer( GL_ARRAY_BUFFER, circleVBO );
    glBufferData( GL_ARRAY_BUFFER, 3 * 13 * sizeof(GLfloat), (GLfloat*)&circlePoints, GL_STATIC_DRAW );
    glEnableVertexAttribArray( primitiveShader.positionAttribute );
    glVertexAttribPointer( primitiveShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint circleIBO;
    glGenBuffers( 1, &circleIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, circleIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 14 * sizeof(GLuint), (GLuint*)&circleIndicies, GL_STATIC_DRAW );

    glDrawElements( GL_TRIANGLE_FAN, 14, GL_UNSIGNED_INT, NULL );  

    glDeleteBuffers( 1, &circleVBO );
    glDeleteBuffers( 1, &circleIBO );
    glUseProgram( (GLuint)NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

bool LoadTextureFromFile( OpenGLTexture* texData, const char* fileName ) {
    //Load data from file
    int n;
    unsigned char* data = stbi_load( fileName, &texData->width, &texData->height, &n, 0 );
    if( data == NULL ) {
        printf( "Could not load file: %s\n", fileName );
        return false;
    }
    printf( "Loaded file: %s\n", fileName );
    printf( "Width: %d, Height: %d, Components per channel: %d\n", texData->width, texData->height, n );

    if( n == 3 ) {
        texData->pixelFormat = GL_RGB;
    } else if( n == 4 ) {
        texData->pixelFormat = GL_RGBA;
    }

    //Generate Texture and bind pointer
    glGenTextures( 1, &texData->textureID );
    glBindTexture( GL_TEXTURE_2D, texData->textureID );
    printf( "Created texture from file %s with id: %d\n", fileName, texData->textureID );

    //Set texture wrapping
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //Set Texture filtering
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); // Use nearest filtering all the time
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    glTexImage2D( GL_TEXTURE_2D, 0, n, texData->width, texData->height, 0, texData->pixelFormat, GL_UNSIGNED_BYTE, data );

    glBindTexture( GL_TEXTURE_2D, 0 );
    stbi_image_free( data );

    return true;
}

bool CreateEmptyTexture( OpenGLTexture* texture, const uint16_t width, const uint16_t height ) {
    if(texture == NULL) {
        return false;
    }

    size_t spaceNeeded = sizeof(GLuint) * width * height;
    if( ( texture->data = (GLuint*)malloc( spaceNeeded ) ) == NULL) {
        return false;
    }
    memset( texture->data, 0, spaceNeeded );

    texture->height = height;
    texture->width = width;
    texture->pixelFormat = GL_RGBA;

    //Generate Texture and bind pointer
    glGenTextures( 1, &texture->textureID );
    if( texture->textureID == -1 ){
        return false;
    }

    glBindTexture( GL_TEXTURE_2D, texture->textureID );

    //Set texture wrapping
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //Set Texture filtering
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); // Use nearest filtering all the time
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    //Create Texture in Memory, parameters in order:
    //1 - Which Texture binding
    //2 - # of MipMaps
    //3 - # of components per pixel, 4 means RGBA, 3 means RGB & so on
    //4 - Texture Width
    //5 - Texture Height
    //6 - OpenGL Reference says this value must be 0 :I whatta great library
    //7 - How are pixels organized
    //8 - size of each component
    //9 - ptr to texture data
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data );

    glBindTexture( GL_TEXTURE_2D, 0 );

    return true;
}

void CreateFrameBuffer( OpenGLFramebuffer* buffer ) {
    CreateEmptyTexture( &buffer->framebufferTexture, 640, 480 );

    glGenFramebuffers( 1, &buffer->framebufferPtr );   

    const GLuint iData[6] = { 0, 1, 2, 2, 3, 0 };
    const GLfloat vData[4 * 3] = {-1.0f, -1.0f, 0.0f, 
                            1.0f, -1.0f, 0.0f,
                            1.0f, 1.0f, 0.0f, 
                            -1.0f, 1.0f, 0.0f };
    const GLfloat uvData[4 * 2] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    //Create VBO
    glGenBuffers( 1, &buffer->framebuffVBO );
    glBindBuffer( GL_ARRAY_BUFFER, buffer->framebuffVBO );
    glBufferData( GL_ARRAY_BUFFER, 3 * 4 * sizeof(GLfloat), &vData, GL_STATIC_DRAW );

    //Create IBO
    glGenBuffers( 1, &buffer->framebuffIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffer->framebuffIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), &iData, GL_STATIC_DRAW );

    //Create UV Buffer
    glGenBuffers( 1, &buffer->framebuffUVBuff );
    glBindBuffer( GL_ARRAY_BUFFER, buffer->framebuffUVBuff );
    glBufferData( GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), &uvData, GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );
}

void RenderFramebuffer( OpenGLFramebuffer* framebuffer ) {
    glUseProgram( framebufferShader.programID );

    glBindBuffer( GL_ARRAY_BUFFER, framebuffer->framebuffVBO );
    glEnableVertexAttribArray( framebufferShader.positionAttribute );
    glVertexAttribPointer( framebufferShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer( GL_ARRAY_BUFFER, framebuffer->framebuffUVBuff );
    glEnableVertexAttribArray( framebufferShader.texCoordAttribute );
    glVertexAttribPointer( framebufferShader.texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    glBindTexture( GL_TEXTURE_2D, framebuffer->framebufferTexture.textureID );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, framebuffer->framebuffIBO );    //Bind index data
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL );     //Render da quad

    //clear shader
    glUseProgram( (GLuint)NULL );

    //Unbind textures
    glBindTexture( GL_TEXTURE_2D, 0 );
}

void printShaderLog( GLuint shader ) {
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

void CreateShader(ShaderProgram* program, const char* vertShaderFile, const char* fragShaderFile) {
    const size_t shaderSrcBufferLength = 1200;
    GLchar shaderSrcBuffer[shaderSrcBufferLength];
    memset(&shaderSrcBuffer, 0, sizeof(GLchar) * shaderSrcBufferLength );
    const GLchar* bufferPtr = (GLchar*)&shaderSrcBuffer;

    program->programID = glCreateProgram();

    GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
    //TODO: Error handling on this
    uint16_t readReturnCode = ReadShaderSrcFileFromDisk( vertShaderFile, (GLchar*)&shaderSrcBuffer, shaderSrcBufferLength );
    assert(readReturnCode == 0);
    glShaderSource( vertexShader, 1, &bufferPtr, NULL );

    glCompileShader( vertexShader );
    GLint compiled = GL_FALSE;
    glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Could not compile Vertex Shader\n" );
        printShaderLog( vertexShader );
    } else {
        printf( "Vertex Shader %s compiled\n", vertShaderFile );
        glAttachShader( program->programID, vertexShader );
    }

    memset(&shaderSrcBuffer, 0, sizeof(char) * shaderSrcBufferLength );

    GLuint fragShader = glCreateShader( GL_FRAGMENT_SHADER );
    //TODO: Error handling on this
    readReturnCode = ReadShaderSrcFileFromDisk( fragShaderFile, (GLchar*)&shaderSrcBuffer, shaderSrcBufferLength );
    assert(readReturnCode == 0);
    glShaderSource( fragShader, 1, &bufferPtr, NULL );

    glCompileShader( fragShader );
    //Check for errors
    compiled = GL_FALSE;
    glGetShaderiv( fragShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Unable to compile fragment shader\n" );
        printShaderLog( fragShader );
    } else {
        printf( "Frag Shader %s compiled\n", fragShaderFile );
        //Actually attach it if it compiled
        glAttachShader( program->programID, fragShader );
    }

    glLinkProgram( program->programID );
    //Check for errors
    compiled = GL_TRUE;
    glGetProgramiv( program->programID, GL_LINK_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Error linking program\n" );
    } else {
        printf( "Shader Program Linked Successfully\n");
    }

    glUseProgram(program->programID);

    program->modelMatrixUniformPtr = glGetUniformLocation( program->programID, "modelMatrix" );
    program->isArmatureAnimatedUniform = glGetUniformLocation( program->programID, "isSkeletalAnimated" );
    program->skeletonUniform = glGetUniformLocation( program->programID, "skeleton" );

    program->positionAttribute = glGetAttribLocation( program->programID, "position" );
    program->texCoordAttribute = glGetAttribLocation( program->programID, "texCoord" );
    program->boneWeightsAttribute = glGetAttribLocation( program->programID, "boneWeights" );
    program->boneIndiciesAttribute = glGetAttribLocation( program->programID, "boneIndicies" );

    program->samplerPtrs[0] = glGetUniformLocation( program->programID, "tex1" );
    program->samplerPtrs[1] = glGetUniformLocation( program->programID, "tex2" );
    glUniform1i( program->samplerPtrs[0], 0 );
    glUniform1i( program->samplerPtrs[1], 1 );
    program->samplerCount = 2;

    glUseProgram(0);
    glDeleteShader( vertexShader ); 
    glDeleteShader( fragShader );
}

bool LoadMeshFromFile( MeshData* data, Skeleton* skeleton, ArmatureKeyframe* animKey, const char* fileName ) {
    // Start the import on the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll t
    // probably to request more postprocessing than we do in this example.
    unsigned int myFlags = 
    aiProcess_GenSmoothNormals         |
    aiProcess_JoinIdenticalVertices    | // join identical vertices/ optimize indexing
    aiProcess_ImproveCacheLocality     | // improve the cache locality of the output vertices
    aiProcess_FindDegenerates          | // remove degenerated polygons from the import
    aiProcess_FindInvalidData          | // detect invalid model data, such as invalid normal vectors
    aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
    aiProcess_LimitBoneWeights         | // limit bone weights to 4 per vertex
    0;

    const struct aiScene* scene = aiImportFile( fileName, myFlags );

    // If the import failed, report it
    if( !scene ) {
        printf( "Failed to import %s\n", fileName );
        return false;
    }

    // Now we can access the file's contents
    uint16_t numMeshes = scene->mNumMeshes;
    printf( "Loaded file: %s\n", fileName );
    if( numMeshes == 0 ) {
        return false;
    }
    printf( "%d Meshes in file\n", numMeshes );

    //uint16_t indexIndirectionList[ MAXVERTCOUNT ];
    //memset( &indexIndirectionList, 0, sizeof(uint16_t) * MAXVERTCOUNT );

    //Read data for each mesh
    for( uint8_t i = 0; i < numMeshes; i++ ) {
        const aiMesh* mesh = scene->mMeshes[i];
        printf( "Mesh %d: %d Vertices, %d Faces\n", i, mesh->mNumVertices, mesh->mNumFaces );

        static float highest_y = 0.0f;
        uint16_t vertexCount = mesh->mNumVertices;
        for( uint16_t j = 0; j < vertexCount; j++ ) {
            data->vertexData[j * 3 + 0] = mesh->mVertices[j].x;
            data->vertexData[j * 3 + 1] = mesh->mVertices[j].z;
            data->vertexData[j * 3 + 2] = -mesh->mVertices[j].y;
            //printf( "Vertex Data %d: %f, %f, %f\n", j, data->vertexData[j * 3 + 0], data->vertexData[j * 3 + 1], data->vertexData[j * 3 + 2]);

            if( mesh->GetNumUVChannels() != 0 ) {
                data->uvData[j * 2 + 0] = mesh->mTextureCoords[0][j].x;
                data->uvData[j * 2 + 1] = mesh->mTextureCoords[0][j].y;
                //printf("UV Data: %f, %f\n", data->uvData[j * 2 + 0], data->uvData[j * 2 + 1]);
            }

            if( mesh->mVertices[j].z > highest_y ) highest_y = mesh->mVertices[j].z;
        }
        data->vertexCount = vertexCount;

        printf( "Highest base y is: %2f\n", highest_y );

        //printf("Unique Data Count: %d\n", uniqueDataCount);
        //for( auto it = uniqueVertIndexLookup.begin(); it != uniqueVertIndexLookup.end(); it++ ) {
            //printf("x:%2f y:%2f z:%2f u:%2f v:%2f\n", it->first.x, it->first.y, it->first.z, it->first.u, it->first.v);
        //}

        uint16_t indexCount = mesh->mNumFaces * 3;
        //printf("Index Pointer data: %p\n", data->indexData );
        for( uint16_t j = 0; j < mesh->mNumFaces; j++ ) {
            const struct aiFace face = mesh->mFaces[j];
            data->indexData[j * 3 + 0] = face.mIndices[0];
            data->indexData[j * 3 + 1] = face.mIndices[1];
            data->indexData[j * 3 + 2] = face.mIndices[2];
            //printf( "Index Set: %d, %d, %d\n", data->indexData[j * 3 + 0], data->indexData[j * 3 + 1], data->indexData[j * 3 + 2]);
        }
        data->indexCount = indexCount;


        if( mesh->HasBones() ) {
            printf( "This mesh also has %d bones\n", mesh->mNumBones );
            data->hasSkeletonInfo = true;
            data->skeleton = skeleton;
            data->skeleton->boneCount = mesh->mNumBones;

            //A map and comparator, for tracking which nodes in the heirarchy are related to which bones
            struct strcompare{
                bool operator() (const char* lhs, const char* rhs) const {
                    return (strcmp(lhs, rhs) < 0);
                }
            };
            std::map<char*, aiNode*, strcompare> nodesByName;
            std::map<char*, Bone*, strcompare> bonesByName;

            uint8_t bonesInfluencingEachVert [ MAXVERTCOUNT * MAXBONEPERVERT ];
            memset( &bonesInfluencingEachVert, 0, sizeof(uint8_t) * MAXVERTCOUNT * MAXBONEPERVERT );
            memset( &data->boneWeightData, 0.0f, sizeof(GLfloat) * MAXVERTCOUNT * MAXBONEPERVERT );
            memset( &data->boneIndexData, 0, sizeof(GLuint) * MAXVERTCOUNT * MAXBONEPERVERT );

            for( uint8_t j = 0; j < data->skeleton->boneCount; j++ ) {
                const aiBone* bone = mesh->mBones[j];
                const uint16_t numVertsAffected = bone->mNumWeights;
                Bone* myBone = &data->skeleton->allBones[j];
                myBone->boneIndex = j;
                myBone->transformMatrix = &data->skeleton->boneTransforms[j];
                Identity( myBone->transformMatrix );

                memcpy( &myBone->name, &bone->mName.data, bone->mName.length * sizeof(char) );
                printf( "Copying info for bone: %s, affects %d verts\n", myBone->name, numVertsAffected );

                //Insert this bones node in the map, for creating hierarchy later
                aiNode* boneNode = scene->mRootNode->FindNode( &myBone->name[0] );
                nodesByName.insert( {myBone->name, boneNode} );
                bonesByName.insert( {myBone->name, myBone} );

                //Set weight data for verticies affected by this bone
                for( uint16_t k = 0; k < numVertsAffected; k++ ) {
                    const aiVertexWeight weightInfo = bone->mWeights[k];
                    //Index into the arrays that hold data, each 4 consecutive indicies corresponds to one vertex
                    //So vertex ID * MAXBONECOUNT + num of bones that have already influenced to point = index
                    const uint16_t indexToWeightData = MAXBONEPERVERT * weightInfo.mVertexId + bonesInfluencingEachVert[weightInfo.mVertexId];

                    data->boneWeightData[indexToWeightData] = weightInfo.mWeight;
                    data->boneIndexData[indexToWeightData] = myBone->boneIndex;
                    bonesInfluencingEachVert[weightInfo.mVertexId]++;
                    assert(bonesInfluencingEachVert[weightInfo.mVertexId] <= 4);
                }
            }

            for( uint8_t j = 0; j < data->skeleton->boneCount; j++ ) {
                Bone* bone = &data->skeleton->allBones[j];
                aiNode* correspondingNode = nodesByName.find( &bone->name[0] )->second;

                printf( "Building hierarchy data for bone: %s\n", bone->name );

                aiNode* parentNode = correspondingNode->mParent;
                auto bone_it = bonesByName.find( parentNode->mName.data );
                if( bone_it != bonesByName.end() ) {
                    bone->parentBone = bone_it->second;
                    printf("Parent Set, parent bone name %s, parent node name %s\n", bone->parentBone->name, parentNode->mName.data);
                } else {
                    printf("No parent found for bone %s\n", bone->name);
                    bone->parentBone = NULL;
                    data->skeleton->rootBone = bone;
                }

                bone->childCount = 0;
                for( uint8_t k = 0; k < correspondingNode->mNumChildren; k++ ) {
                    aiNode* childNode = correspondingNode->mChildren[k];
                    auto childBone_it = bonesByName.find( childNode->mName.data );
                    if( childBone_it != bonesByName.end() ) {
                        Bone* childBone = childBone_it->second;
                        bone->childrenBones[bone->childCount] = childBone;
                        bone->childCount++;
                    }
                }
                printf( "%d children found for this bone\n", bone->childCount );
            }

        } else {
            data->hasSkeletonInfo = false;
            printf( "No bone info\n" );
        }
    }

    if( scene->HasAnimations() ) {
        ArmatureKeyframe* sPose = animKey;
        sPose->skeleton = data->skeleton;
        const aiAnimation* anim = scene->mAnimations[0];

        const uint8_t bonesInAnimation = anim->mNumChannels;
        for( uint8_t i = 0; i < bonesInAnimation; i++ ) {
            const aiNodeAnim* boneAnimation = anim->mChannels[i];

            Bone* bone = NULL;
            for( uint8_t j = 0; j < data->skeleton->boneCount; j++ ) {
                int result = strcmp( data->skeleton->allBones[j].name, boneAnimation->mNodeName.data );
                if( result == 0 ) {
                    bone = &data->skeleton->allBones[j];
                    break;
                }
            }

            if( bone != NULL ) {
                BoneKey* key = &sPose->keys[bone->boneIndex];
                key->boneAffected = bone;

                aiVector3D veckey1 = boneAnimation->mPositionKeys[0].mValue;
                aiQuaternion quatKey1 = boneAnimation->mRotationKeys[0].mValue;
                aiVector3D scaleKey1 = boneAnimation->mScalingKeys[0].mValue;

                key->scale = { scaleKey1.x, scaleKey1.z, -scaleKey1.y };
                key->rotation = { quatKey1.w, quatKey1.x, quatKey1.y, quatKey1.z };
                key->translation = { veckey1.x, veckey1.y, veckey1.z };

                if(bone->boneIndex == 0) {
                    Quaternion inverse = key->rotation;
                    Inverse( &inverse );
                    key->rotation = MultQuats( key->rotation, inverse );
                }
                printf("Bone scale values - x:%2f, y:%2f, z:%2f\n", key->scale.x, key->scale.y, key->scale.z );
                printf("Bone Quat values - w:%2f, x:%2f, y:%2f, z:%2f\n", key->rotation.w, key->rotation.x, key->rotation.y, key->rotation.z );
                printf("Bone translate values - x:%2f, y:%2f, z:%2f\n", key->translation.x, key->translation.y, key->translation.z );

                Vec3 axis;
                float angle;
                ToAngleAxis( key->rotation, &angle, &axis );
                angle *= 180.0f / 3.1415926f;
                printf( "For this quat angle axis is - angle:%2f, axis - x:%2f, y:%2f, z:%2f\n", angle, axis.x, axis.y, axis.z );
            }
        }
    } else {
        printf( "No Animation info found\n" );
    }

    aiReleaseImport( scene );
    return true; 
}

void SetSkeletonTransform( ArmatureKeyframe* key, Skeleton* skeleton ) {
    struct N {
        static void _SetBoneTransformRecursively( ArmatureKeyframe* key, Bone* bone, Matrix4 parentMatrix ) {
            BoneKey* bKey = &key->keys[bone->boneIndex];

            Matrix4 scaleMatrix, rotationMatrix, translationMatrix;
            SetScale( &scaleMatrix, bKey->scale.x, bKey->scale.y, bKey->scale.z );
            rotationMatrix = MatrixFromQuat( bKey->rotation );
            SetTranslation( &translationMatrix, bKey->translation.x, bKey->translation.y, bKey->translation.z );

            Matrix4 netMatrix = translationMatrix * rotationMatrix * scaleMatrix;
            *bone->transformMatrix = netMatrix * parentMatrix;

            for( uint8_t i = 0; i < bone->childCount; i++ ) {
                _SetBoneTransformRecursively( key, bone->childrenBones[i], *bone->transformMatrix );
            }
        }
    };

    Matrix4 i;
    Identity( &i );
    N::_SetBoneTransformRecursively( key, skeleton->rootBone, i );

    // int8_t stackIndex = -1;
    // uint8_t childrenTraversed[MAXBONECOUNT];
    // Bone* boneStack[MAXBONECOUNT];

    // boneStack[0] = skeleton->rootBone;
    // stackIndex = 0;

    // //while( stackIndex >= 0 ) {
    //     Bone* currentBone = boneStack[stackIndex];
    //     BoneKey* bKey = &key->keys[currentBone->boneIndex];

    //     Matrix4 scaleMatrix, rotationMatrix, translationMatrix;
    //     SetScale( &scaleMatrix, bKey->scale.x, bKey->scale.y, bKey->scale.z );
    //     rotationMatrix = MatrixFromQuat( bKey->rotation );
    //     SetTranslation( &translationMatrix, bKey->translation.x, bKey->translation.y, bKey->translation.z );

    //     Matrix4 netMatrix = scaleMatrix * rotationMatrix * translationMatrix;
    // //}
}

void CreateRenderMesh(OpenGLMeshBinding* renderMesh, MeshData* meshData) {
    renderMesh->modelMatrix = &meshData->modelMatrix;

    glGenBuffers( 1, &renderMesh->vbo );
    glBindBuffer( GL_ARRAY_BUFFER, renderMesh->vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * meshData->vertexCount * sizeof(GLfloat), meshData->vertexData, GL_STATIC_DRAW );

    glGenBuffers( 1, &renderMesh->ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, renderMesh->ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, meshData->indexCount * sizeof(GLuint), meshData->indexData, GL_STATIC_DRAW );

    glGenBuffers( 1, &renderMesh->uvBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, renderMesh->uvBuffer );
    glBufferData( GL_ARRAY_BUFFER, 2 * meshData->vertexCount * sizeof(GLfloat), meshData->uvData, GL_STATIC_DRAW );

    renderMesh->isArmatureAnimated = meshData->hasSkeletonInfo;
    if( meshData->hasSkeletonInfo ) {
        printf("Added skeleton info to GL mesh\n");
        renderMesh->skeleton = meshData->skeleton;

        glGenBuffers(1, &renderMesh->boneWeightBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, renderMesh->boneWeightBuffer );
        glBufferData( GL_ARRAY_BUFFER, MAXBONEPERVERT * meshData->vertexCount * sizeof(GLfloat), meshData->boneWeightData, GL_STATIC_DRAW );

        glGenBuffers(1, &renderMesh->boneIndexBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, renderMesh->boneIndexBuffer );
        glBufferData( GL_ARRAY_BUFFER, MAXBONEPERVERT * meshData->vertexCount * sizeof(GLuint), meshData->boneIndexData, GL_STATIC_DRAW );

        //SetSkeletonTransform( key,  );
    }

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );

    renderMesh->vertexCount = meshData->indexCount;
}

void RenderMesh( OpenGLMeshBinding* mesh, ShaderProgram* program, ShaderTextureSet textureSet ) {
    //Flush errors
    //while( glGetError() != GL_NO_ERROR ){};

    //Bind Shader
    glUseProgram( program->programID );
    glUniformMatrix4fv( program->modelMatrixUniformPtr, 1, false, (float*)mesh->modelMatrix->m[0] );
    glUniform1i( program->isArmatureAnimatedUniform, (int)mesh->isArmatureAnimated );

    //Set vertex data
    glBindBuffer( GL_ARRAY_BUFFER, mesh->vbo );
    glEnableVertexAttribArray( program->positionAttribute );
    glVertexAttribPointer( program->positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set UV data
    glBindBuffer( GL_ARRAY_BUFFER, mesh->uvBuffer );
    glEnableVertexAttribArray( program->texCoordAttribute );
    glVertexAttribPointer( program->texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    if( mesh->isArmatureAnimated ) {
        glUniformMatrix4fv( program->skeletonUniform, MAXBONECOUNT, GL_FALSE, (float*)&mesh->skeleton->boneTransforms->m[0] );

        glBindBuffer( GL_ARRAY_BUFFER, mesh->boneWeightBuffer );
        glEnableVertexAttribArray( program->boneWeightsAttribute );
        glVertexAttribPointer( program->boneWeightsAttribute, MAXBONEPERVERT, GL_FLOAT, false, 0, 0 );

        glBindBuffer( GL_ARRAY_BUFFER, mesh->boneIndexBuffer );
        glEnableVertexAttribArray( program->boneIndiciesAttribute );
        glVertexAttribIPointer( program->boneIndiciesAttribute, MAXBONEPERVERT, GL_UNSIGNED_INT, 0, 0 );
    }

    glBindTexture( GL_TEXTURE_2D, textureSet.texturePtrs[0] );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, textureSet.texturePtrs[1] );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->ibo );                           //Bind index data
    glDrawElements( GL_TRIANGLES, mesh->vertexCount, GL_UNSIGNED_INT, NULL );     ///Render, assume its all triangles

    //Unbind textures
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    //clear shader
    glUseProgram( (GLuint)NULL );
}

bool InitOpenGLRenderer( const float screen_w, const float screen_h ) {
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

    const float halfHeight = screen_h * 0.5f;
    const float halfWidth = screen_w * 0.5f;
    glMatrixMode(GL_PROJECTION);
    glOrtho( -halfWidth, halfWidth, -halfHeight, halfHeight, -halfWidth, halfWidth );

    //Initialize framebuffer
    CreateFrameBuffer( &myFramebuffer );
    CreateShader( &framebufferShader, "Data/Shaders/Framebuffer.vert", "Data/Shaders/Framebuffer.frag" );

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

void GameInit( MemorySlab* gameMemory ) {
    if( InitOpenGLRenderer( 640, 480 ) == false ) {
        printf("Failed to init OpenGL renderer\n");
        return;
    }

    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;
    CreateShader( &primitiveShader, "Data/Shaders/Line.vert", "Data/Shaders/Line.frag" );

    const float localVert[9] = {-300.0f, -200.0f, 1.0f, 20.0f, -100.0f, 0.0f, 300.0f, 200.0f, -1.0f };
    const uint32 localIndex[3] = { 0, 1, 2 };
    memcpy(&gMem->vertexData, &localVert, 9 * sizeof(float) );
    memcpy(&gMem->indexData, &localIndex, 3 * sizeof(uint32) );

    printf("Init went well\n");
    return;
}

void Render( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    glClear( GL_DEPTH_BUFFER_BIT );
    glBindFramebuffer( GL_FRAMEBUFFER, myFramebuffer.framebufferPtr );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, myFramebuffer.framebufferTexture.textureID, 0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderLineStrip( (float*)&gMem->vertexData, (uint32*)&gMem->indexData, 3 );
    RenderCircle( {gMem->x, gMem->y, 0.0f}, 8.0f );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    RenderFramebuffer( &myFramebuffer );
}

bool Update( MemorySlab* gameMemory ) {
    GameMemory* gMem = (GameMemory*)gameMemory->slabStart;

    float x, y;
    GetMousePosition( &x, &y );
    if( x < -1.0f || x > 1.0f ) x = 0.0f;
    if( y < -1.0f || y > 1.0f ) y = 0.0f;

    x *= (640 / 2);
    y *= (480 / 2);

    gMem->x = x;
    gMem->y = y;

    return true;
}