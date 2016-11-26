#include "Math3D.h"
#include "Armatures.h"

struct MeshGeometryData;
struct ShaderProgram;
struct RenderCommand;
struct RenderCommand_Interleaved;
struct TextureData;

typedef uint32 PtrToGpuMem;

struct RenderDriver {
    bool (*ParseMeshDataFromCollada)( void*, Stack*, MeshGeometryData*, Armature* );
    PtrToGpuMem (*AllocNewGpuArray)( void );
	PtrToGpuMem (*CopyVertexDataToGpuMem)( void*, size_t );
    void (*CopyDataToGpuArray)( PtrToGpuMem, void*, uint64 );
    void (*CopyTextureDataToGpuMem)( TextureData*, PtrToGpuMem* );
	void (*CreateShaderProgram)( char*, char*, ShaderProgram* );
    void (*ClearShaderProgram)( ShaderProgram* );
	void (*Draw)( RenderCommand*, bool, bool );
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

struct RenderCommand {
    enum {
        INTERLEAVESTREAM, SEPARATE_GPU_ARRAYS
    };

	ShaderProgram* shader;

	uint32 elementCount;

    uint8 vertexFormat;
    union {
        uint32 vertexInputData [ MAX_SUPPORTED_VERT_INPUTS ];
        struct {
            void* streamData;
            size_t streamSize;
            size_t vertSize;
            PtrToGpuMem bufferForStream;
            uint8 vertexAttributeOffsets [ MAX_SUPPORTED_VERT_INPUTS ];
        } VertexFormat;
    };

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
//View Ranges on this matrix are exactly xyz: width, height, depth
static void CreateOrthoCameraMatrix( 
	float width, 
	float height, 
    float depth,
	Mat4* m
) {
    m->m[0][0] = 1.0f / width; m->m[0][1] = 0.0f; m->m[0][2] = 0.0f; m->m[0][3] = 0.0f;
    m->m[1][0] = 0.0f; m->m[1][1] = -1.0f / height; m->m[1][2] = 0.0f; m->m[1][3] = 0.0f;
    m->m[2][0] = 0.0f; m->m[2][1] = 0.0f; m->m[2][2] = 1.0f / depth; m->m[2][3] = 0.0f;
    m->m[3][0] = 0.0f; m->m[3][1] = 0.0f; m->m[3][2] = 0.0f; m->m[3][3] = 1.0f;
}

static Mat4 CreatePerspectiveMatrix( 
    float fov, 
    float aspect, 
    float nearPlane,
    float farPlane,
    float scale
) {
    float depth = farPlane - nearPlane;
    float inverseTanFov = 1.0 / tanf( fov / 2.0 );

    return {
        ( inverseTanFov / aspect ) / scale, 0.0f, 0.0f, 0.0f,
        0.0f, -inverseTanFov / scale, 0.0f, 0.0f,
        0.0f, 0.0f, ( ( farPlane + nearPlane ) / depth ) / scale, 1.0f,
        0.0f, 0.0f, ( ( farPlane * nearPlane ) / depth ), 1.0f
    };
}

//A NOTE viewprojection is MultMatrix( view, projection )

static Mat4 CreateViewMatrix( Quat rotation, Vec3 p ) {
    Vec3 xAxis = ApplyQuatToVec( rotation, { 1.0f, 0.0f, 0.0f } );
    Vec3 yAxis = ApplyQuatToVec( rotation, { 0.0f, 1.0f, 0.0f } );
    Vec3 zAxis = ApplyQuatToVec( rotation, { 0.0f, 0.0f, 1.0f } );

    return {
        xAxis.x, yAxis.x, zAxis.x, 0.0f,
        xAxis.y, yAxis.y, zAxis.y, 0.0f,
        xAxis.z, yAxis.z, zAxis.z, 0.0f,
        -Dot( xAxis, p ), -Dot( yAxis, p ), -Dot( zAxis, p ), 1.0f
    };
}

static Mat4 CreateViewMatrix( Vec3 p, Vec3 lookAtTarget ) {
    Vec3 up = { 0.0f, 1.0f, 0.0f };
    Vec3 lookDirection = lookAtTarget - p;
    Normalize( &lookDirection );

    Vec3 zAxis = lookDirection;
    Vec3 xAxis = Cross( lookDirection, up );
    Vec3 yAxis = Cross( zAxis, xAxis );

    return {
        xAxis.x, yAxis.x, zAxis.x, 0.0f,
        xAxis.y, yAxis.y, zAxis.y, 0.0f,
        xAxis.z, yAxis.z, zAxis.z, 0.0f,
        -Dot( xAxis, p ), -Dot( yAxis, p ), -Dot( zAxis, p ), 1.0f
    };
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

static int GetIndexOfProgramUniformInput( ShaderProgram* shader, char* inputName ) {
    for( int i = 0; i < shader->uniformCount; ++i ) {
        if( strcmp( shader->uniformNames[i] , inputName ) == 0 ) {
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

    driver->CreateShaderProgram( vertSrc, fragSrc, bindDataStorage );

    FreeFromStack( allocater, (void*)fragSrc );
    FreeFromStack( allocater, (void*)vertSrc );
}
#endif