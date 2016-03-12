static ShaderProgram primitiveShader;
//THIS IS FOR DEBUGGING PURPOSES ONLY, DON'T SHIP WITH THIS, BAD PROGRAMMING PRACTICE WITHIN
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

    glDrawElements( GL_LINES, bufferCount, GL_UNSIGNED_INT, NULL );

    glDeleteBuffers( 1, &lineVBO );
    glDeleteBuffers( 1, &lineIBO );
    glUseProgram( (GLuint)NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

//THIS IS FOR DEBUGGING PURPOSES ONLY, DON'T SHIP WITH THIS, BAD PROGRAMMING PRACTICE WITHIN
void RenderCircle( Vec3 position, float radius, Vec3 color ) {
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

    Mat4 translateMatrix, scaleMatrix, netMatrix;
    SetToIdentity( &translateMatrix );
    SetToIdentity( &scaleMatrix );
    SetTranslation( &translateMatrix, position.x, position.y, position.z );
    SetScale( &scaleMatrix, radius, radius, radius );
    netMatrix = scaleMatrix * translateMatrix;
    glUniformMatrix4fv( primitiveShader.modelMatrixUniformPtr, 1, false, (float*)&netMatrix );

    GLfloat float4[4] = {color.x, color.y, color.z, 1.0f };
    glUniform4f( glGetUniformLocation( primitiveShader.programID, "primitiveColor"), color.x, color.y, color.z, 1.0f );

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


#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb\stb_image.h"

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

    assert( buffer->framebuffIBO != 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffer->framebuffIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), &iData, GL_STATIC_DRAW );

    //Create UV Buffer
    glGenBuffers( 1, &buffer->framebuffUVBuff );
    glBindBuffer( GL_ARRAY_BUFFER, buffer->framebuffUVBuff );
    glBufferData( GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), &uvData, GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );
}

static ShaderProgram framebufferShader;

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
    const size_t shaderSrcBufferLength = 1700;
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
    program->cameraMatrixUniformPtr = glGetUniformLocation( program->programID, "cameraMatrix" );
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

#include "assimp\cimport.h"               // Assimp Plain-C interface
#include "assimp\scene.h"                 // Assimp Output data structure
#include "assimp\postprocess.h"           // Assimp Post processing flags
bool LoadMeshViaAssimp( MeshData* data, Skeleton* skeleton, ArmatureKeyframe* animKey, const char* fileName ) {
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

    aiMatrix4x4t<float> rootTransform = scene->mRootNode->mTransformation;
    aiVector3t<float> tScale;
    aiVector3t<float> tPosition;
    aiQuaterniont<float> tRotation;
    rootTransform.Decompose( tScale, tRotation, tPosition );

    float ang;
    Vec3 ax;
    ToAngleAxis( {tRotation.w, tRotation.x, tRotation.y, tRotation.z}, &ang, &ax );
    Mat4 rootSMat, rootTMat, rootRMat;
    SetToIdentity( &rootSMat ); SetToIdentity( &rootTMat ); SetToIdentity( &rootRMat );
    SetRotation( &rootRMat, ax.z, ax.y, ax.x, ang );
    SetTranslation( &rootTMat, tPosition.x, tPosition.y, tPosition.z );
    SetScale( &rootSMat, tScale.x, tScale.y, tScale.z );
    Mat4 rootTransformMat = rootSMat * rootRMat * rootTMat;
    Mat4 rootInverseTransform = InverseMatrix( rootTransformMat );

    // printf( "Transform data for%s\n", scene->mRootNode->mName.data );
    // printf( "Position x:%.3f, y:%.3f, z:%.3f\n", tPosition.x, tPosition.y, tPosition.z );
    // printf( "Scale x:%.3f, y:%.3f, z:%.3f\n", tScale.x, tScale.y, tScale.z );
    // printf( "Rotation -- Angle:%.3f -- Axis: x:%.3f, y:%.3f, z:%.3f\n", ang, ax.x, ax.y, ax.z );

    // Now we can access the file's contents
    uint16_t numMeshes = scene->mNumMeshes;
    printf( "Loaded file: %s\n", fileName );
    if( numMeshes == 0 ) {
        return false;
    }
    printf( "%d Meshes in file\n", numMeshes );

    //Read data for each mesh
    for( uint8_t i = 0; i < numMeshes; i++ ) {
        const aiMesh* mesh = scene->mMeshes[i];
        printf( "Mesh %d: %d Vertices, %d Faces\n", i, mesh->mNumVertices, mesh->mNumFaces );

        uint16_t vertexCount = mesh->mNumVertices;
        for( uint16_t j = 0; j < vertexCount; j++ ) {
            data->vertexData[j * 3 + 0] = mesh->mVertices[j].x;
            data->vertexData[j * 3 + 1] = mesh->mVertices[j].z;
            data->vertexData[j * 3 + 2] = -mesh->mVertices[j].y;

            if( mesh->GetNumUVChannels() != 0 ) {
                data->uvData[j * 2 + 0] = mesh->mTextureCoords[0][j].x;
                data->uvData[j * 2 + 1] = mesh->mTextureCoords[0][j].y;
            }
        }
        data->vertexCount = vertexCount;

        uint16_t indexCount = mesh->mNumFaces * 3;
        for( uint16_t j = 0; j < mesh->mNumFaces; j++ ) {
            const struct aiFace face = mesh->mFaces[j];
            data->indexData[j * 3 + 0] = face.mIndices[0];
            data->indexData[j * 3 + 1] = face.mIndices[1];
            data->indexData[j * 3 + 2] = face.mIndices[2];
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
                memcpy( &myBone->name, &bone->mName.data, bone->mName.length * sizeof(char) );

                //Insert this bones node in the map, for creating hierarchy later
                aiNode* boneNode = scene->mRootNode->FindNode( &myBone->name[0] );
                nodesByName.insert( {myBone->name, boneNode} );
                bonesByName.insert( {myBone->name, myBone} );

                myBone->transformMatrix = &data->skeleton->boneTransforms[j];
                SetToIdentity( myBone->transformMatrix );

                aiVector3t<float> bindScale;
                aiVector3t<float> bindPosition;
                aiQuaterniont<float> bindRotation;
                bone->mOffsetMatrix.Decompose( bindScale, bindRotation, bindPosition );

                float a;
                Vec3 v;
                ToAngleAxis( { bindRotation.w, bindRotation.x, bindRotation.y, bindRotation.z }, &a, &v );
                myBone->bindRotation = FromAngleAxis( v.z, v.y, v.x, a );
                myBone->bindPosition = { bindPosition.x, -bindPosition.y, bindPosition.z };
                myBone->bindScale = { bindScale.x, bindScale.y, bindScale.z };

                //Debug Print info, I expect I will need this again eventually
                // printf("Bind Data %s\n", myBone->name);
                // printf("Translation --"); PrintVec( myBone->bindPosition );
                // printf("\nScale --"); PrintVec( myBone->bindScale );
                // float angle;
                // Vec3 axis;
                // ToAngleAxis( myBone->bindRotation, &angle, &axis);
                // printf("\nRotation -- Angle: %.3f ", angle); PrintVec(axis);
                // printf("\n");


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
                aiNode* parentNode = correspondingNode->mParent;
                auto bone_it = bonesByName.find( parentNode->mName.data );
                if( bone_it != bonesByName.end() ) {
                    bone->parentBone = bone_it->second;
                } else {
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
            }

            struct N {
                static void SetBindMatxs( Bone* bone, Mat4 parentBindMatrix ) {
                    Mat4 bindTranslationMat, bindRotationMat, bindScaleMat;

                    SetToIdentity( &bindTranslationMat ); SetToIdentity( &bindScaleMat );
                    bindRotationMat = MatrixFromQuat( bone->bindRotation );
                    SetScale( &bindScaleMat, bone->bindScale.x, bone->bindScale.y, bone->bindScale.z );
                    SetTranslation( &bindTranslationMat, bone->bindPosition.x, bone->bindPosition.y, bone->bindPosition.z );

                    Mat4 netMatrix = bindScaleMat * bindRotationMat * bindTranslationMat;
                    bone->bindMatrix = parentBindMatrix * netMatrix;
                    bone->inverseBindMatrix = InverseMatrix( bone->bindMatrix );

                    for( uint8 childBoneIndex = 0; childBoneIndex < bone->childCount; childBoneIndex++ ) {
                        SetBindMatxs( bone->childrenBones[childBoneIndex], bone->bindMatrix );
                    }
                };
            };

            Mat4 i;
            SetToIdentity( &i ) ;
            N::SetBindMatxs( data->skeleton->rootBone, rootInverseTransform );

        } else {
            data->hasSkeletonInfo = false;
            printf( "No bone info\n" );
        }
    }

    if( scene->HasAnimations() ) {
        printf("This file also has an animation\n");
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

                if(bone->boneIndex == 0) {
                    key->scale = { scaleKey1.x * (1.0f / tScale.x), scaleKey1.y * (1.0f / tScale.y), scaleKey1.z * (1.0f / tScale.z)};
                    key->translation = { veckey1.x - tPosition.x, veckey1.y - tPosition.y, veckey1.z - tPosition.z };
                    key->rotation = MultQuats( { quatKey1.w, quatKey1.x, quatKey1.y, quatKey1.z }, 
                                                InverseQuat( { tRotation.w, tRotation.x, tRotation.y, tRotation.z } ) );
                } else {
                    key->scale = { scaleKey1.x, scaleKey1.y, scaleKey1.z };
                    key->rotation = { quatKey1.w, quatKey1.x, quatKey1.y, quatKey1.z };
                    key->translation = { veckey1.x, veckey1.y, veckey1.z };
                }
            }
        }
    } else {
        printf( "No Animation info found\n" );
    }

    aiReleaseImport( scene );
    return true; 
}

void CreateGLMeshBinding(OpenGLMeshBinding* renderMesh, MeshData* meshData) {
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
        renderMesh->skeleton = meshData->skeleton;

        glGenBuffers(1, &renderMesh->boneWeightBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, renderMesh->boneWeightBuffer );
        glBufferData( GL_ARRAY_BUFFER, MAXBONEPERVERT * meshData->vertexCount * sizeof(GLfloat), meshData->boneWeightData, GL_STATIC_DRAW );

        glGenBuffers(1, &renderMesh->boneIndexBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, renderMesh->boneIndexBuffer );
        glBufferData( GL_ARRAY_BUFFER, MAXBONEPERVERT * meshData->vertexCount * sizeof(GLuint), meshData->boneIndexData, GL_STATIC_DRAW );
    }

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );

    renderMesh->vertexCount = meshData->indexCount;
}

void RenderGLMeshBinding( Mat4* cameraMatrix, OpenGLMeshBinding* mesh, ShaderProgram* program, ShaderTextureSet textureSet ) {
    //Flush errors
    //while( glGetError() != GL_NO_ERROR ){};

    //Bind Shader
    glUseProgram( program->programID );
    glUniformMatrix4fv( program->modelMatrixUniformPtr, 1, false, (float*)mesh->modelMatrix->m[0] );
    glUniformMatrix4fv( program->cameraMatrixUniformPtr, 1, false, (float*)cameraMatrix->m[0] );
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