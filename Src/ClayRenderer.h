#ifndef CLAY_RENDERER


/* -------------------------------------
GENERIC STRUCTS FOR THE GAME APP TO USE
----------------------------------------*/

struct Bone {
    uint8_t boneIndex;
    Matrix4* transformMatrix;
    Bone* parentBone;
    Bone* childrenBones[4];
    uint8_t childCount;
    char name[32];
};

struct Skeleton {
    #define MAXBONECOUNT 32
    Bone allBones[MAXBONECOUNT];
    Matrix4 boneTransforms[MAXBONECOUNT];
    uint8_t boneCount;
    Bone* rootBone;
};

struct MeshData {
    #define MAXVERTCOUNT 600
    #define MAXBONEPERVERT 4
    Matrix4 modelMatrix;
    uint16_t indexCount;
    uint16_t vertexCount;
    GLfloat vertexData [MAXVERTCOUNT * 3];
    GLfloat uvData[MAXVERTCOUNT * 2];

    bool hasSkeletonInfo;
    Skeleton* skeleton;
    GLfloat boneWeightData[MAXVERTCOUNT * MAXBONEPERVERT];
    GLuint boneIndexData[MAXVERTCOUNT * MAXBONEPERVERT];
    GLuint indexData [600];
};

/*-------------------------------------
IMPLEMENTATIONS SPECIFIC TO THE RENDERER
---------------------------------------*/
void InitRenderer();
//void* CreateRenderableMesh( MeshData* genericMeshData );
void RenderMesh( void* rendererSpecificMesh );

#define CLAY_RENDERER
#endif