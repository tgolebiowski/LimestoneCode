#ifndef CLAY_RENDERER


/* -------------------------------------
GENERIC STRUCTS FOR THE GAME APP TO USE
----------------------------------------*/

struct Bone {
    uint8_t boneIndex;
    
    Vec3 bindPosition;
    Vec3 bindScale;
    Quat bindRotation;

    Mat4 bindMatrix;
    Mat4 inverseBindMatrix;

    Mat4* transformMatrix;
    Bone* parentBone;
    Bone* childrenBones[4];
    uint8_t childCount;
    char name[32];
};

struct Skeleton {
    #define MAXBONECOUNT 32
    Bone allBones[MAXBONECOUNT];
    Mat4 boneTransforms[MAXBONECOUNT];
    uint8_t boneCount;
    Bone* rootBone;
};

struct BoneKey {
    Bone* boneAffected;
    Vec3 scale;
    Quat rotation;
    Vec3 translation;
};

struct ArmatureKeyframe {
    Skeleton* skeleton;
    BoneKey keys[MAXBONECOUNT];
};

struct MeshData {
    #define MAXVERTCOUNT 600
    #define MAXBONEPERVERT 4
    Mat4 modelMatrix;
    uint16_t indexCount;
    uint16_t vertexCount;
    GLfloat vertexData [MAXVERTCOUNT * 3];
    GLfloat uvData[MAXVERTCOUNT * 2];

    bool hasSkeletonInfo;
    Skeleton* skeleton;
    GLfloat boneWeightData[MAXVERTCOUNT * MAXBONEPERVERT];
    GLuint boneIndexData[MAXVERTCOUNT * MAXBONEPERVERT];
    GLuint indexData [MAXVERTCOUNT * 3];

    bool hasAnimationInfo;
    ArmatureKeyframe debugFrame;
};

/*-------------------------------------
IMPLEMENTATIONS SPECIFIC TO THE RENDERER
---------------------------------------*/
//void InitRenderer();
//void* CreateRenderableMesh( MeshData* genericMeshData );
//void RenderMesh( void* rendererSpecificMesh );
//void RenderLine( float* lineVertexBuffer, uint16 vertexCount );

#define CLAY_RENDERER
#endif