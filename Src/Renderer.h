#include "Math3D.h"

struct MeshGeometryData {
	#define MAXVERTCOUNT 600
	Vec3 vData[ MAXVERTCOUNT ];
	float uvData[ MAXVERTCOUNT * 2 ];
	Vec3 normalData[ MAXVERTCOUNT ];
	uint32 iData[ 600 ];
	uint32 dataCount;
};

struct MeshSkinningData {
	#define MAXBONESPERVERT 4
	float boneWeightData[ MAXVERTCOUNT * MAXBONESPERVERT ];
	uint8 boneIndexData[ MAXVERTCOUNT * MAXBONESPERVERT ];
};

struct TextureData {
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
	Bone* rootBone;
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

struct RendererThings {
	//Data for rendering lines as a debugging tool
	float lineData[64];
	ShaderProgramBinding pShader;
	uint32 lineDataPtr;
	uint32 lineIDataPtr;

};
static RendererThings rendererStorage;

void CreateEmptyTexture( TextureData* texDataStorage, uint16 width, uint16 height );

/*-----------------------------------------------------------------------------------------------------------------
                                    THINGS FOR THE RENDERER TO IMPLEMENT
------------------------------------------------------------------------------------------------------------------*/

bool InitRenderer( uint16 screen_w, uint16 screen_h );
void CreateRenderBinding( MeshGPUBinding* bindData, MeshGeometryData* geometryStorage, MeshSkinningData* skinningStorage = NULL );
void CreateShaderProgram( ShaderProgramBinding* bindData, const char* vertProgramFilePath, const char* fragProgramFilePath );
void CreateTextureBinding( TextureBindingID* texBindID, TextureData* textureData );

void RenderBoundData( MeshGPUBinding* renderBinding, ShaderProgramBinding* program, ShaderProgramParams params );

void RenderDebugLines( float* vertexData, uint8 vertexCount );
void RenderArmatureAsLines( Armature* armature );

//TODO: figure out if this is useful or not
//Hypothetical ease-of-use idea, you always want renderbinding but only sometimes care to store the mesh data anywhere other
//than on the GPU, then this function header below would be easiest
//LoadRenderBinding( const char fileName, MeshRenderBinding** bindDataStorage, MeshDataStorage** meshDataStorage = NULL );

/*------------------------------------------------------------------------------------------------------------------
                                       THINGS FOR THE OS TO IMPLEMENT
--------------------------------------------------------------------------------------------------------------------*/

///Return 0 on success, required buffer length if buffer is too small, or -1 on other OS failure
int16 ReadShaderSrcFileFromDisk(const char* fileName, char* buffer, uint16 bufferLen);
void LoadMeshDataFromDisk( const char* fileName, MeshGeometryData* storage );
void LoadMeshSkinningDataFromDisk( const char* fileName, MeshSkinningData* storage, Armature* armature = NULL );
void LoadTextureDataFromDisk( const char* fileName, TextureData* texDataStorage );