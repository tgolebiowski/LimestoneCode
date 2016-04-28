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
	Mat4 bindPose, invBindPose;
	Mat4* currentTransform;
	Bone* parent;
	Bone* children[4];
	char name[32];
	uint8 childCount;
	uint8 boneIndex;
};

struct Armature {
	#define MAXBONES 32
	Bone allBones[ MAXBONES ];
	Mat4 boneTransforms[ MAXBONES ];
	Bone* rootBone;
	uint8 boneCount;
};

struct BoneKeyFrame {
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

void SetOrthoProjectionMatrix( float width, float height, float nearPlane, float farPlane ) {
    Mat4* m = &rendererStorage.baseProjectionMatrix;
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    float depth = nearPlane - farPlane;

    m->m[0][0] = 1.0f / halfWidth; m->m[0][1] = 0.0f; m->m[0][2] = 0.0f; m->m[0][3] = 0.0f;
    m->m[1][0] = 0.0f; m->m[1][1] = 1.0f / halfHeight; m->m[1][2] = 0.0f; m->m[1][3] = 0.0f;
    m->m[2][0] = 0.0f; m->m[2][1] = 0.0f; m->m[2][2] = 2.0f / depth; m->m[2][3] = 0.0f;
    m->m[3][0] = 0.0f; m->m[3][1] = 0.0f; m->m[3][2] = -(farPlane + nearPlane) / depth; m->m[3][3] = 1.0f;
}

void ApplyArmaturePose( Armature* armature, ArmatureKeyFrame* pose ) {
	struct {
		static void ApplyPoseRecursive( Bone* bone, Mat4 parentTransform, ArmatureKeyFrame* pose ) {
			BoneKeyFrame boneKey = pose->localBoneTransforms[ bone->boneIndex ];
			*bone->currentTransform = MultMatrix( Mat4FromComponents( boneKey.position, boneKey.scale, boneKey.rotation ), parentTransform );

			for( uint8 childIndex = 0; childIndex < bone->childCount; childIndex++ ) {
				ApplyPoseRecursive( bone->children[ childIndex ], *bone->currentTransform, pose );
			}

			*bone->currentTransform = MultMatrix( bone->invBindPose, *bone->currentTransform );
		};
	}LocalFunctions;

	Mat4 i; SetToIdentity( &i );
	LocalFunctions.ApplyPoseRecursive( armature->rootBone, i, pose );
}

void CreateEmptyTexture( TextureData* texDataStorage, uint16 width, uint16 height ) {
	texDataStorage->texData = ( uint8* )malloc( sizeof( uint8 ) * 4 * width * height );
    texDataStorage->width = width;
    texDataStorage->height = height;
    texDataStorage->channelsPerPixel = 4;
}

/*-----------------------------------------------------------------------------------------------------------------
                                    THINGS FOR THE RENDERER TO IMPLEMENT
------------------------------------------------------------------------------------------------------------------*/

bool InitRenderer( uint16 screen_w, uint16 screen_h );
void CreateTextureBinding( TextureData* textureData, TextureBindingID* texBindID );
void CreateShaderProgram( const char* vertProgramFilePath, const char* fragProgramFilePath, ShaderProgramBinding* bindData );
void CreateRenderBinding( MeshGeometryData* geometryStorage, MeshGPUBinding* bindData );
void RenderBoundData( MeshGPUBinding* renderBinding, ShaderProgramBinding* program, ShaderProgramParams params );
void RenderTexturedQuad( TextureBindingID* texture, float width, float height, float x, float y );

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
void LoadAnimationDataFromCollada( const char* fileName, ArmatureKeyFrame* pose, Armature* armature );
void LoadTextureDataFromDisk( const char* fileName, TextureData* texDataStorage );