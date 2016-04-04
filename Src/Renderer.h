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

struct RendererThings {
	Mat4 baseProjectionMatrix;
	Mat4 cameraTransform;

	ShaderProgramBinding pShader;
	//Data for rendering lines as a debugging tool
	uint32 lineDataPtr;
	uint32 lineIDataPtr;
	//Data for rendering circles/dots as a debugging tool
	uint32 circleDataPtr;
	uint32 circleIDataPtr;
};
static RendererThings rendererStorage;

void SetOrthoProjectionMatrix( float width, float height, float nearPlane, float farPlane );
void CreateEmptyTexture( TextureData* texDataStorage, uint16 width, uint16 height );

/*-----------------------------------------------------------------------------------------------------------------
                                    THINGS FOR THE RENDERER TO IMPLEMENT
------------------------------------------------------------------------------------------------------------------*/

bool InitRenderer( uint16 screen_w, uint16 screen_h );
void CreateTextureBinding( TextureBindingID* texBindID, TextureData* textureData );
void CreateShaderProgram( ShaderProgramBinding* bindData, const char* vertProgramFilePath, const char* fragProgramFilePath );
void CreateRenderBinding( MeshGPUBinding* bindData, MeshGeometryData* geometryStorage );
void RenderBoundData( MeshGPUBinding* renderBinding, ShaderProgramBinding* program, ShaderProgramParams params );

void RenderDebugCircle( Vec3 position, float radius = 1.0f , Vec3 color = { 1.0f, 1.0f, 1.0f} );
void RenderDebugLines( float* vertexData, uint8 vertexCount, Mat4 transform, Vec3 color = { 1.0f, 1.0f, 1.0f } );
void RenderArmatureAsLines( Armature* armature, Mat4 transform, Vec3 color = { 1.0f, 1.0f, 1.0f } );

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
void LoadTextureDataFromDisk( const char* fileName, TextureData* texDataStorage );