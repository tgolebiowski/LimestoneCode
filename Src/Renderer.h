#include "Math3D.h"

struct MeshGeometryStorage {
	#define MAXVERTCOUNT 600
	Vec3 vData[ MAXVERTCOUNT ];
	float uvData[ MAXVERTCOUNT * 2 ];
	Vec3 normalData[ MAXVERTCOUNT ];
	uint32 iData[ 600 ];
	uint32 dataCount;
};

struct MeshSkinningStorage {
	#define MAXBONESPERVERT 4
	float boneWeightData[ MAXVERTCOUNT * MAXBONESPERVERT ];
	uint8 boneIndexData[ MAXVERTCOUNT * MAXBONESPERVERT ];
};

struct TextureDataStorage {
	uint8* texData;
	uint16 width;
	uint16 height;
	uint8 channelsPerPixel;
};

typedef uint32 TextureBindingID;

struct Bone {
	Mat4 inverseBindPose;
	Mat4* currentTransform;
	Bone* parent;
	Bone* children[4];
	char name[32];
	uint8 childCount;
};

struct Armature {
	#define MAXBONES 32
	Bone allBones[ MAXBONES ];
	Mat4 boneTransforms[ MAXBONES ];
	uint8 boneCount;
};

struct MeshGPUBinding {
	uint32 dataCount;
	uint32 vertexDataPtr;
	uint32 indexDataPtr;
	uint32 uvDataPtr;

	//uint32 boneWeightDataPtr;
	//uint32 boneIndexDataPtr;
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
//Hypothetical ease-of-use idea, you always want renderbinding but only sometimes care to store the mesh data anywhere other
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
void CreateRenderBinding( MeshGPUBinding* bindDataStorage, MeshGeometryStorage* geometryStorage, MeshSkinningStorage* skinningStorage = NULL );
void CreateShaderProgram( ShaderProgramBinding* bindDataStorage, const char* vertProgramFilePath, const char* fragProgramFilePath );
void CreateTextureBinding( TextureBindingID* texBindID, TextureDataStorage* textureData );

void RenderBoundData( MeshGPUBinding* renderBinding, ShaderProgramBinding* program, ShaderProgramParams params );

/*------------------------------------------------------------------------------------------------------------------
                                       THINGS FOR THE OS TO IMPLEMENT
--------------------------------------------------------------------------------------------------------------------*/

///Return 0 on success, required buffer length if buffer is too small, or -1 on other OS failure
int16 ReadShaderSrcFileFromDisk(const char* fileName, char* buffer, uint16 bufferLen);
void LoadMeshDataFromDisk( const char* fileName, MeshGeometryStorage* storage );
void LoadMeshSkinningDataFromDisk( const char* fileName, MeshSkinningStorage* storage, Armature* armature = NULL );
void LoadTextureDataFromDisk( const char* fileName, TextureDataStorage* dataStorage );