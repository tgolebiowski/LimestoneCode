#include "Math3D.h"
#include "Armatures.h"

struct MeshGeometryData;
struct ShaderProgram;
struct RenderCommand;
struct RenderCommand_Interleaved;
struct TextureData;

typedef uint32 PtrToGpuMem;

struct RenderDriver {
	PtrToGpuMem (*CopyVertexDataToGpuMem)( RenderDriver*, void*, size_t );
	void (*CreateShaderProgram)( RenderDriver*, char*, char*, ShaderProgram* );
    void (*ClearShaderProgram)( RenderDriver*, ShaderProgram* );
	void (*DrawMesh)( RenderDriver*, RenderCommand* );
    void (*DrawInterleavedStream)( RenderDriver*, RenderCommand_Interleaved* );
	bool (*ParseMeshDataFromCollada)( void*, Stack*, MeshGeometryData*, Armature*);
    void (*CopyTextureDataToGpuMem)( RenderDriver*, TextureData*, PtrToGpuMem* );
    PtrToGpuMem (*AllocGpuBuffForDynamicStream)( RenderDriver* );
};

#define MAXBONESPERVERT 4
struct MeshGeometryData {
	Vec3* vData;
	Vec3* normalData;
	float* uvData;
	float* boneWeightData;
	uint32* boneIndexData;
	uint32 dataCount;
};

struct TextureData {
	uint8* data;
	uint16 width;
	uint16 height;
	uint8 channelsPerPixel;
};

#define MAX_SUPPORTED_VERT_INPUTS 16
#define MAX_SUPPORTED_UNIFORMS 16
#define MAX_SUPPORTED_TEX_SAMPLERS 4
struct ShaderProgram {
	uint32 programID;
    char nameBuffer [512];
    char* vertexInputNames[ MAX_SUPPORTED_VERT_INPUTS ];
    char* uniformNames[ MAX_SUPPORTED_UNIFORMS ];
    char* samplerNames[ MAX_SUPPORTED_TEX_SAMPLERS ];

    int32 vertexInputPtrs[ MAX_SUPPORTED_VERT_INPUTS ];
    int32 uniformPtrs[ MAX_SUPPORTED_UNIFORMS ];
    int32 samplerPtrs[ MAX_SUPPORTED_TEX_SAMPLERS ];

    int32 vertexInputTypes[ MAX_SUPPORTED_VERT_INPUTS ];
    int32 uniformTypes[ MAX_SUPPORTED_UNIFORMS ];
    uint8 vertInputCount, uniformCount, samplerCount;
};

struct RenderCommand_Interleaved {
    ShaderProgram* shader;
    uint32 elementCount;

    void* streamData;
    size_t streamSize;
    size_t vertSize;
    PtrToGpuMem bufferForStream;
    uint8 vertexAttributeOffsets [ MAX_SUPPORTED_VERT_INPUTS ];
    PtrToGpuMem sampleData[ MAX_SUPPORTED_VERT_INPUTS ];
};

struct RenderCommand {
	ShaderProgram* shader;

	uint32 elementCount;

    uint32 vertexInputData [ MAX_SUPPORTED_VERT_INPUTS ];
    void* uniformData[ MAX_SUPPORTED_UNIFORMS ];
    PtrToGpuMem samplerData[ MAX_SUPPORTED_TEX_SAMPLERS ];
};

struct Framebuffer {
    enum FramebufferType {
        DEPTH, COLOR
    };
    FramebufferType type;
    uint32 framebufferPtr;
    TextureData framebufferTexture;
    PtrToGpuMem textureBindingID;
};

#ifdef DLL_ONLY
void SetRendererCameraProjection( 
	float width, 
	float height, 
	float farPlane, 
	float nearPlane, 
	Mat4* m 
) {
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    float depth = farPlane - nearPlane;

    m->m[0][0] = 1.0f / halfWidth; m->m[0][1] = 0.0f; m->m[0][2] = 0.0f; m->m[0][3] = 0.0f;
    m->m[1][0] = 0.0f; m->m[1][1] = 1.0f / halfHeight; m->m[1][2] = 0.0f; m->m[1][3] = 0.0f;
    m->m[2][0] = 0.0f; m->m[2][1] = 0.0f; m->m[2][2] = -2.0f / depth; m->m[2][3] = 0.0f;
    m->m[3][0] = 0.0f; m->m[3][1] = 0.0f; m->m[3][2] = -(nearPlane + farPlane) / depth; m->m[3][3] = 1.0f;
}

static void CreateEmptyTexture(
    TextureData* texData,
    uint16 width,
    uint16 height,
    Stack* allocater
) {
    size_t texDataSize = sizeof( uint8 ) * 4 * width * height;
	texData->data = ( uint8* )StackAlloc( allocater, texDataSize );
    texData->width = width;
    texData->height = height;
    texData->channelsPerPixel = 4;
}

static void SetSampler( 
    RenderCommand* params, 
    char* targetSamplerName, 
    PtrToGpuMem texBinding 
) {
    for( int samplerIndex = 0; samplerIndex < params->shader->samplerCount; ++samplerIndex ) {
        if( strcmp( params->shader->samplerNames[ samplerIndex ], targetSamplerName ) == 0 ) {
            params->samplerData[ samplerIndex ] = texBinding;
            return;
        }
    }

    printf( "Cannot set sampler named: %s because it couldn't be found\n", targetSamplerName );
}

static void SetVertexInput( 
    RenderCommand* params, 
    char* targetInputName, 
    PtrToGpuMem gpuDataPtr 
) {
    for( int vertexInputIndex = 0; 
        vertexInputIndex < params->shader->vertInputCount; 
        ++vertexInputIndex ) 
    {
        if( strcmp( 
            params->shader->vertexInputNames[ vertexInputIndex ], 
            targetInputName ) == 0 
        ) {
            params->vertexInputData[ vertexInputIndex ] = gpuDataPtr;
            return;
        }
    }

    printf( 
        "Cannot set vertex input named: %s because it couldn't be found\n", 
        targetInputName 
    );
}

static void SetUniform( 
    RenderCommand* params, 
    char* uniformName, 
    void* uniformData 
) {
    for( 
    	int uniformNamesIndex = 0; 
    	uniformNamesIndex < params->shader->uniformCount; 
    	++uniformNamesIndex 
    ) {
        if( strcmp( params->shader->uniformNames[ uniformNamesIndex ], uniformName ) == 0 ) {
            params->uniformData[ uniformNamesIndex ] = uniformData;
            return;
        }
    }

    printf( "Cannot set uniform named: %s because it couldn't be found\n", uniformName );
}

static int32 GetShaderProgramInputPtr( ShaderProgram* shader, char* inputName ) {
    for( int i = 0; i < shader->vertInputCount; ++i ) {
        if( strcmp( shader->vertexInputNames[i] , inputName ) == 0 ) {
            return shader->vertexInputPtrs[i];
        }
    }

    for( int i = 0; i < shader->uniformCount; ++i ) {
        if( strcmp( shader->uniformNames[i] , inputName ) == 0 ) {
            return shader->uniformPtrs[i];
        }
    }

    for( int i = 0; i < shader->samplerCount; ++i ) {
        if( strcmp( shader->samplerNames[i] , inputName ) == 0 ) {
            return shader->samplerPtrs[i];
        }
    }

    return -1;
}

static int GetIndexOfProgramVertexInput( ShaderProgram* shader, char* inputName ) {
    for( int i = 0; i < shader->vertInputCount; ++i ) {
        if( strcmp( shader->vertexInputNames[i] , inputName ) == 0 ) {
            return i;
        }
    }
}

static void CreateShaderProgramFromSourceFiles(
    RenderDriver* driver,
    char* vertProgramFilePath, 
    char* fragProgramFilePath, 
    ShaderProgram* bindDataStorage,
    Stack* allocater,
    System* system
) {
    char* vertSrc = (char*)system->ReadWholeFile( vertProgramFilePath, allocater );
    char* fragSrc = (char*)system->ReadWholeFile( fragProgramFilePath, allocater );

    driver->CreateShaderProgram( driver, vertSrc, fragSrc, bindDataStorage );

    FreeFromStack( allocater, (void*)fragSrc );
    FreeFromStack( allocater, (void*)vertSrc );
}
#endif