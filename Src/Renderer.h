#ifndef RENDERER_H
#define RENDERER_H
#include "Math3D.h"

struct MeshGeometryData {
	#define MAXVERTCOUNT 600
	Vec3 vData[ MAXVERTCOUNT ];
	float uvData[ MAXVERTCOUNT * 2 ];
	Vec3 normalData[ MAXVERTCOUNT ];
	#define MAXBONESPERVERT 4
	bool hasBoneData;
	float boneWeightData[ MAXVERTCOUNT * MAXBONESPERVERT ];
	uint32 boneIndexData[ MAXVERTCOUNT * MAXBONESPERVERT ];
	uint32 iData[ 600 ];
	uint32 dataCount;
};

struct TextureData {
	uint8* data;
	uint16 width;
	uint16 height;
	uint8 channelsPerPixel;
};

typedef uint32 TextureBindingID;

#define MAXBONES 32
struct Bone {
	Mat4 bindPose, invBindPose;
	Mat4* currentTransform;
	Bone* parent;
	Bone* children[4];
	char name[32];
	uint8 childCount;
	uint8 boneIndex;
};

struct Armature {
	Bone allBones[ MAXBONES ];
	Mat4 boneTransforms[ MAXBONES ];
	Bone* rootBone;
	uint8 boneCount;
};

struct BoneKeyFrame {
	//Mat4 rawMatrix;
	Vec3 position, scale;
	Quat rotation;
};

struct ArmatureKeyFrame {
	BoneKeyFrame localBoneTransforms[ MAXBONES ];
};

struct MeshGPUBinding {
	uint32 dataCount;
	uint32 vertexDataPtr;
	uint32 indexDataPtr;
	uint32 uvDataPtr;

	bool hasBoneData;
	uint32 boneWeightDataPtr;
	uint32 boneIndexDataPtr;
};

struct ShaderProgramBinding {
	uint32 programID;
    int32 modelMatrixUniformPtr;
    int32 cameraMatrixUniformPtr;
    int32 armatureUniformPtr;

    int32 positionAttribute;
    int32 texCoordAttribute;

    int32 isArmatureAnimated;
    int32 boneWeightsAttribute;
    int32 boneIndiciesAttribute;

    int32 samplerPtr1;
    int32 samplerPtr2;
};

struct ShaderProgramParams {
	Mat4* modelMatrix;
	TextureBindingID sampler1;
	TextureBindingID sampler2;
	Armature* armature;
};

static struct {
	Mat4 baseProjectionMatrix;
	Mat4 cameraTransform;

	ShaderProgramBinding texturedQuadShader;
	uint32 quadVDataPtr;
	uint32 quadUVDataPtr;
	uint32 quadIDataPtr;

	ShaderProgramBinding pShader;
	//Data for rendering lines as a debugging tool
	uint32 lineDataPtr;
	uint32 lineIDataPtr;
	//Data for rendering circles/dots as a debugging tool
	uint32 circleDataPtr;
	uint32 circleIDataPtr;
} rendererStorage;

/*----------------------------------------------------------------------------------------------------------------
                                      PLATFORM INDEPENDENT FUNCTIONS
-----------------------------------------------------------------------------------------------------------------*/

void ApplyArmatureKeyFrame( ArmatureKeyFrame* pose, Armature* armature, bool doPrint ) {
	struct {
		static void ApplyKeyFrameRecursive( Bone* bone, Mat4 parentTransform, ArmatureKeyFrame* pose, bool doPrint ) {
			BoneKeyFrame* boneKey = &pose->localBoneTransforms[ bone->boneIndex ];

			if( doPrint ) {
				printf( "%s\n position: %f, %f, %f\n scale %f, %f, %f\n rotation %f, %f, %f, %f\n", bone->name, boneKey->position.x, boneKey->position.y, boneKey->position.z,
					boneKey->scale.x, boneKey->scale.y, boneKey->scale.z, boneKey->rotation.w, boneKey->rotation.x, boneKey->rotation.y, boneKey->rotation.z );
			}

			Mat4 baseComponentMat = Mat4FromComponents( boneKey->scale, boneKey->rotation, boneKey->position );
			Mat4 resultComponentMat = MultMatrix( baseComponentMat, parentTransform );
			*bone->currentTransform = resultComponentMat;

			for( uint8 childIndex = 0; childIndex < bone->childCount; childIndex++ ) {
				ApplyKeyFrameRecursive( bone->children[ childIndex ], *bone->currentTransform, pose, doPrint );
			}

			*bone->currentTransform = MultMatrix( bone->invBindPose, *bone->currentTransform );
		};
	}LocalFunctions;

	Mat4 i; SetToIdentity( &i );
	LocalFunctions.ApplyKeyFrameRecursive( armature->rootBone, i, pose, doPrint );
}

void ApplyBlendedArmatureKeyFrames( uint8 keyframeCount, ArmatureKeyFrame** keyframes, float* weights, Armature* armature, bool doPrint ) {
	struct {
		static void ApplyBlendedKeyFramesRecursive( uint8 keyframeCount, ArmatureKeyFrame** keyframes, float* weights, Mat4* parentTransforms, Bone* target, bool doPrint ) {
			Mat4 worldTransforms[4];
			for( uint8 keyframeIndex = 0; keyframeIndex < keyframeCount; keyframeIndex++ ) {
				BoneKeyFrame* bonekey = &keyframes[ keyframeIndex ]->localBoneTransforms[ target->boneIndex ];
				Mat4 localBoneMat = Mat4FromComponents( bonekey->scale, bonekey->rotation, bonekey->position );
				Mat4 worldBoneMat = MultMatrix( localBoneMat, parentTransforms[ keyframeIndex ] );
				worldTransforms[ keyframeIndex ] = worldBoneMat;
			}

			for( uint8 childIndex = 0; childIndex < target->childCount; childIndex++ ) {
				ApplyBlendedKeyFramesRecursive( keyframeCount, keyframes, weights, &worldTransforms[0], target->children[ childIndex ], doPrint );
			}

			BoneKeyFrame localKey = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f } };
			for( uint8 keyframeIndex = 0; keyframeIndex < keyframeCount; keyframeIndex++ ) {
				BoneKeyFrame calculatedKey;
				DecomposeMat4( worldTransforms[ keyframeIndex ], &calculatedKey.scale, &calculatedKey.rotation, &calculatedKey.position );

				localKey.position.x += weights[ keyframeIndex ] * calculatedKey.position.x;
				localKey.position.y += weights[ keyframeIndex ] * calculatedKey.position.y;
				localKey.position.z += weights[ keyframeIndex ] * calculatedKey.position.z;
				localKey.scale.x += weights[ keyframeIndex ] * calculatedKey.scale.x;
				localKey.scale.y += weights[ keyframeIndex ] * calculatedKey.scale.y;
				localKey.scale.z += weights[ keyframeIndex ] * calculatedKey.scale.z;

				localKey.rotation = Slerp( localKey.rotation, calculatedKey.rotation, weights[ keyframeIndex ] );

				//localKey.rotation.w += weights[ keyframeIndex ] * calculatedKey.rotation.w;
 			    //localKey.rotation.x += weights[ keyframeIndex ] * calculatedKey.rotation.x;
				//localKey.rotation.y += weights[ keyframeIndex ] * calculatedKey.rotation.y;
				//localKey.rotation.z += weights[ keyframeIndex ] * calculatedKey.rotation.z;
			}

			//float rotLen = sqrtf( localKey.rotation.w * localKey.rotation.w + localKey.rotation.x * localKey.rotation.x + 
			//localKey.rotation.y * localKey.rotation.y + localKey.rotation.z * localKey.rotation.z );
			//localKey.rotation.w /= rotLen;
			//localKey.rotation.x /= rotLen;
			//localKey.rotation.y /= rotLen;
			//localKey.rotation.z /= rotLen;

			if( doPrint )
				printf( "%s\n position: %f, %f, %f\n scale %f, %f, %f\n rotation %f, %f, %f, %f\n", target->name, localKey.position.x, localKey.position.y, localKey.position.z,
				localKey.scale.x, localKey.scale.y, localKey.scale.z, localKey.rotation.w, localKey.rotation.x, localKey.rotation.y, localKey.rotation.z );

			Mat4 netMatrix = Mat4FromComponents( localKey.scale, localKey.rotation, localKey.position );
			*target->currentTransform = MultMatrix( target->invBindPose, netMatrix );
		}
	} LocalFunctions;
	Mat4 i[4]; SetToIdentity( &i[0] ); SetToIdentity( &i[1] ); SetToIdentity( &i[2] ); SetToIdentity( &i[3] );
	LocalFunctions.ApplyBlendedKeyFramesRecursive( keyframeCount, keyframes, weights, &i[0], armature->rootBone, doPrint );
}

void CreateEmptyTexture( TextureData* texData, uint16 width, uint16 height ) {
	texData->data = ( uint8* )malloc( sizeof( uint8 ) * 4 * width * height );
    texData->width = width;
    texData->height = height;
    texData->channelsPerPixel = 4;
}


void SetRendererCameraProjection( float width, float height, float nearPlane, float farPlane ) {
    Mat4* m = &rendererStorage.baseProjectionMatrix;
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    float depth = nearPlane - farPlane;

    m->m[0][0] = 1.0f / halfWidth; m->m[0][1] = 0.0f; m->m[0][2] = 0.0f; m->m[0][3] = 0.0f;
    m->m[1][0] = 0.0f; m->m[1][1] = 1.0f / halfHeight; m->m[1][2] = 0.0f; m->m[1][3] = 0.0f;
    m->m[2][0] = 0.0f; m->m[2][1] = 0.0f; m->m[2][2] = 2.0f / depth; m->m[2][3] = 0.0f;
    m->m[3][0] = 0.0f; m->m[3][1] = 0.0f; m->m[3][2] = -(farPlane + nearPlane) / depth; m->m[3][3] = 1.0f;
}

void SetRendererCameraTransform( Vec3 position, Vec3 lookatTarget ) {
	Mat4 cameraTransform = LookAtMatrix( position, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } );
    SetTranslation( &cameraTransform, position.x, position.y, position.z );
    rendererStorage.cameraTransform = MultMatrix( cameraTransform, rendererStorage.baseProjectionMatrix );
}

/*-----------------------------------------------------------------------------------------------------------------
                                    THINGS FOR THE RENDERER TO IMPLEMENT
------------------------------------------------------------------------------------------------------------------*/

void CreateTextureBinding( TextureData* textureData, TextureBindingID* texBindID );
void CreateShaderProgram( const char* vertProgramFilePath, const char* fragProgramFilePath, ShaderProgramBinding* bindData );
void CreateRenderBinding( MeshGeometryData* geometryStorage, MeshGPUBinding* bindData );

bool InitRenderer( uint16 screen_w, uint16 screen_h );
void RenderBoundData( MeshGPUBinding* renderBinding, ShaderProgramBinding* program, ShaderProgramParams params );
void RenderTexturedQuad( TextureBindingID* texture, float width, float height, float x, float y );

void RenderDebugCircle( Vec3 position, float radius = 1.0f , Vec3 color = { 1.0f, 1.0f, 1.0f} );
void RenderDebugLines( float* vertexData, uint8 vertexCount, Mat4 transform, Vec3 color = { 1.0f, 1.0f, 1.0f } );
void RenderArmatureAsLines(  Armature* armature, Mat4 transform, Vec3 color = { 1.0f, 1.0f, 1.0f } );

//TODO: figure out if this is useful or not
//Hypothetical ease-of-use idea, you always want renderbinding but only sometimes care to store the mesh data anywhere other
//than on the GPU, then this function header below would be easiest
//LoadRenderBinding( const char fileName, MeshRenderBinding** bindDataStorage, MeshDataStorage** meshDataStorage = NULL );

/*------------------------------------------------------------------------------------------------------------------
                                       THINGS FOR THE OS LAYER TO IMPLEMENT
--------------------------------------------------------------------------------------------------------------------*/

///Return 0 on success, required buffer length if buffer is too small, or -1 on other OS failure
int16 ReadShaderSrcFileFromDisk(const char* fileName, char* buffer, uint16 bufferLen);
void LoadMeshDataFromDisk( const char* fileName, MeshGeometryData* storage, Armature* armature = NULL );
void LoadAnimationDataFromCollada( const char* fileName, ArmatureKeyFrame* pose, Armature* armature );
void LoadTextureDataFromDisk( const char* fileName, TextureData* texDataStorage );
#endif //RENDERER_H

#ifdef OPENGL_RENDERER_IMPLEMENTATION

void CreateTextureBinding( TextureData* texData, TextureBindingID* texBindID ) {
	GLenum pixelFormat;
    if( texData->channelsPerPixel == 3 ) {
        pixelFormat = GL_RGB;
    } else if( texData->channelsPerPixel == 4 ) {
        pixelFormat = GL_RGBA;
    }

    GLuint glTextureID;
    glGenTextures( 1, &glTextureID );
    glBindTexture( GL_TEXTURE_2D, glTextureID );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    glTexImage2D( GL_TEXTURE_2D, 0, texData->channelsPerPixel, texData->width, texData->height, 0, pixelFormat, GL_UNSIGNED_BYTE, texData->data );

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

void CreateShaderProgram( const char* vertProgramFilePath, const char* fragProgramFilePath, ShaderProgramBinding* bindDataStorage ) {
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

    glUseProgram( bindDataStorage->programID );

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

void CreateRenderBinding( MeshGeometryData* meshDataStorage, MeshGPUBinding* bindDataStorage ) {
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
    glClearColor( 0.1f, 0.1f, 0.18f, 1.0f );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glViewport( 0.0f, 0.0f, screen_w, screen_h );

    SetToIdentity( &rendererStorage.baseProjectionMatrix );
    SetToIdentity( &rendererStorage.cameraTransform );

    CreateShaderProgram( "Data/Shaders/TexturedQuad.vert", "Data/Shaders/TexturedQuad.frag", &rendererStorage.texturedQuadShader );
    //making these not static, break armature pose loading or applying, not sure which and I don't know why, so this is a hack to avoid that
    static GLfloat quadVData[12] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f };
    static GLfloat quadUVData[8] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    //The staticy nonsense baystahds ^^^^
    const GLuint quadIndexData[6] = { 0, 1, 2, 0, 2, 3 };
    GLuint quadDataPtrs [3];
    glGenBuffers( 3, (GLuint*)quadDataPtrs );

    rendererStorage.quadVDataPtr = quadDataPtrs[0];
    glBindBuffer( GL_ARRAY_BUFFER, quadDataPtrs[0] );
    glBufferData( GL_ARRAY_BUFFER, 12 * sizeof( GLfloat ), &quadVData[0], GL_STATIC_DRAW );
    rendererStorage.quadUVDataPtr = quadDataPtrs[1];
    glBindBuffer( GL_ARRAY_BUFFER, quadDataPtrs[1] );
    glBufferData( GL_ARRAY_BUFFER, 8 * sizeof( GLfloat ), &quadUVData[0], GL_STATIC_DRAW );
    rendererStorage.quadIDataPtr = quadDataPtrs[2];
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, quadDataPtrs[2] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof( GLuint ), &quadIndexData[0], GL_STATIC_DRAW );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR ) {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        return false;
    } else {
        printf( "Initialized OpenGL\n" );
    }

    CreateShaderProgram( "Data/Shaders/Primitive.vert", "Data/Shaders/Primitive.frag", &rendererStorage.pShader );

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

void RenderBoundData( MeshGPUBinding* meshBinding, ShaderProgramBinding* programBinding, ShaderProgramParams params ) {
	//Flush errors
    //while( glGetError() != GL_NO_ERROR ){};

    //Bind Shader
    glUseProgram( programBinding->programID );
    glUniformMatrix4fv( programBinding->modelMatrixUniformPtr, 1, false, (float*)&params.modelMatrix->m[0] );
    glUniformMatrix4fv( programBinding->cameraMatrixUniformPtr, 1, false, (float*)&rendererStorage.cameraTransform.m[0] );
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

void RenderTexturedQuad( TextureBindingID* texture, float width, float height, float x, float y ) {
    Mat4 transform, translation, scale; SetToIdentity( &translation ); SetToIdentity( &scale );
    SetScale( &scale, width, height, 1.0f  ); SetTranslation( &translation, x, y, 0.0f );
    transform = MultMatrix( scale, translation );

    glUseProgram( rendererStorage.texturedQuadShader.programID );
    glUniformMatrix4fv( rendererStorage.texturedQuadShader.modelMatrixUniformPtr, 1, false, (float*)&transform );

    //Set vertex data
    glBindBuffer( GL_ARRAY_BUFFER, rendererStorage.quadVDataPtr );
    glEnableVertexAttribArray( rendererStorage.texturedQuadShader.positionAttribute );
    glVertexAttribPointer( rendererStorage.texturedQuadShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set UV data
    glBindBuffer( GL_ARRAY_BUFFER, rendererStorage.quadUVDataPtr );
    glEnableVertexAttribArray( rendererStorage.texturedQuadShader.texCoordAttribute );
    glVertexAttribPointer( rendererStorage.texturedQuadShader.texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, (GLuint)*texture );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, rendererStorage.quadIDataPtr );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL );
}

void RenderDebugCircle( Vec3 position, float radius, Vec3 color ) {
    glUseProgram( rendererStorage.pShader.programID );

    Mat4 p; SetToIdentity( &p );
    SetTranslation( &p, position.x, position.y, position.z ); SetScale( &p, radius, radius, radius );
    glUniformMatrix4fv( rendererStorage.pShader.modelMatrixUniformPtr, 1, false, (float*)&p.m[0] );
    glUniformMatrix4fv( rendererStorage.pShader.cameraMatrixUniformPtr, 1, false, (float*)&rendererStorage.cameraTransform.m[0] );
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
    glUniformMatrix4fv( rendererStorage.pShader.cameraMatrixUniformPtr, 1, false, (float*)&rendererStorage.cameraTransform.m[0] );
    glUniform4f( glGetUniformLocation( rendererStorage.pShader.programID, "primitiveColor" ), color.x, color.y, color.z, 1.0f );

    glEnableVertexAttribArray( rendererStorage.pShader.positionAttribute );
    glBindBuffer( GL_ARRAY_BUFFER, rendererStorage.lineDataPtr );
    glBufferData( GL_ARRAY_BUFFER, dataCount * 3 * sizeof(float), (GLfloat*)vertexData, GL_DYNAMIC_DRAW );
    glVertexAttribPointer( rendererStorage.pShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, rendererStorage.lineIDataPtr );
    glDrawElements( GL_LINES, dataCount, GL_UNSIGNED_INT, NULL );
}

void RenderArmatureAsLines( Armature* armature, Mat4 transform, Vec3 color ) {
    bool isDepthTesting;
    glGetBooleanv( GL_DEPTH_TEST, ( GLboolean* )&isDepthTesting );
    if( isDepthTesting ) {
        glDisable( GL_DEPTH_TEST );
    }

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

    if( isDepthTesting ) {
        glEnable( GL_DEPTH_TEST );
    }
}
#endif