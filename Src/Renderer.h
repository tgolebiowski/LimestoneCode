#include "Math3D.h"

struct MeshDataStorage {
	#define MAXVERTCOUNT 600
	Vec3 vData[ MAXVERTCOUNT ];
	float uvData[ MAXVERTCOUNT * 2 ];
	Vec3 normalData[ MAXVERTCOUNT ];
	uint32 iData[ 600 ];
	uint32 dataCount;
};

struct TextureDataStorage {
	uint32* texData;
	uint16 width;
	uint16 height;
	uint8 pixelFormat;
};

typedef uint32 TextureBindingID;
// struct TextureBinding {
// 	uint32 texBindingID;
// };

struct MeshRenderBinding {
	uint32 dataCount;
	uint32 vertexDataPtr;
	uint32 indexDataPtr;
	uint32 uvDataPtr;
};

struct ShaderProgramBinding {
	uint32 programID;
    int32 modelMatrixUniformPtr;
    int32 cameraMatrixUniformPtr;
    int32 positionAttribute;
    int32 texCoordAttribute;
    int32 samplerPtr1;
    int32 samplerPtr2;
};

struct ShaderProgramParams {
	Mat4* modelMatrix;
	Mat4* cameraMatrix;
	TextureBindingID sampler1;
	TextureBindingID sampler2;
};

//TODO: figure out if this is useful or not
//Hypothetical ease-of-use idea, you want renderbinding but don't care to store the mesh data anywhere other
//than on the GPU, then this function header below would be easiest
//LoadRenderBinding( const char fileName, MeshRenderBinding** bindDataStorage, MeshDataStorage** meshDataStorage = NULL );

struct RendererThings {
	Mat4 projMatrix;
	Mat4 cameraTransformMatrix;
};
static RendererThings rendererStorage;

void CreateEmptyTexture( TextureDataStorage* texDataStorage, uint16 width, uint16 height );

/*-----------------------------------------------------------------------------------------------------------------
                                    THINGS FOR THE RENDERER TO IMPLEMENT
------------------------------------------------------------------------------------------------------------------*/

bool InitRenderer( uint16 screen_w, uint16 screen_h );
void CreateRenderBinding( MeshRenderBinding* bindDataStorage, MeshDataStorage* meshDataStorage );
void CreateShaderProgram( ShaderProgramBinding* bindDataStorage );
void CreateTextureBinding( TextureBindingID* texBindID, TextureDataStorage* textureData );

void RenderBoundData( MeshRenderBinding* renderBinding, ShaderProgramBinding* program, ShaderProgramParams* params );

/*------------------------------------------------------------------------------------------------------------------
                                       THINGS FOR THE OS TO IMPLEMENT
--------------------------------------------------------------------------------------------------------------------*/

///Return 0 on success, required buffer length if buffer is too small, or -1 on other OS failure
int16 ReadShaderSrcFileFromDisk(const char* fileName, char* buffer, uint16 bufferLen);
void LoadMeshData( const char* fileName, MeshDataStorage* meshDataStorage );
void LoadTextureData( const char* fileName, TextureDataStorage* dataStorage );