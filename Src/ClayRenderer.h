#ifndef CLAY_RENDERER
#include "Math3D.h"

/* -------------------------------------
GENERIC STRUCTS AND FUCTIONS FOR THE GAME APP TO USE
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

void SetSkeletonTransform( ArmatureKeyframe* key, Skeleton* skeleton ) {
    struct N {
        static void _SetBoneTransformRecursively( ArmatureKeyframe* key, Bone* bone, Mat4 parentMatrix ) {
            BoneKey* bKey = &key->keys[bone->boneIndex];

            Mat4 scaleMatrix, rotationMatrix, translationMatrix;
            rotationMatrix = MatrixFromQuat( bKey->rotation );
            SetToIdentity( &scaleMatrix ); SetToIdentity( &translationMatrix );
            SetScale( &scaleMatrix, bKey->scale.x, bKey->scale.y, bKey->scale.z );
            SetTranslation( &translationMatrix, bKey->translation.x, bKey->translation.y, bKey->translation.z );

            Mat4 netMatrix = scaleMatrix * rotationMatrix * translationMatrix;
            *bone->transformMatrix = bKey->boneAffected->inverseBindMatrix * netMatrix;

            // printf("Animation Data %s\n", bKey->boneAffected->name);
            // printf("Translation --"); PrintVec(bKey->translation);
            // printf("\nScale --"); PrintVec(bKey->scale);
            // float angle;
            // Vec3 axis;
            // ToAngleAxis( bKey->rotation, &angle, &axis);
            // printf("\nRotation -- Angle: %.3f ", angle); PrintVec(axis);
            // printf("\n");

            for( uint8_t i = 0; i < bone->childCount; i++ ) {
                _SetBoneTransformRecursively( key, bone->childrenBones[i], *bone->transformMatrix );
            }
        }
    };

    Mat4 i;
    SetToIdentity( &i );
    N::_SetBoneTransformRecursively( key, skeleton->rootBone, i );

    // int8_t stackIndex = -1;
    // uint8_t childrenTraversed[MAXBONECOUNT];
    // Bone* boneStack[MAXBONECOUNT];

    // boneStack[0] = skeleton->rootBone;
    // stackIndex = 0;

    // //while( stackIndex >= 0 ) {
    //     Bone* currentBone = boneStack[stackIndex];
    //     BoneKey* bKey = &key->keys[currentBone->boneIndex];

    //     Mat4 scaleMatrix, rotationMatrix, translationMatrix;
    //     SetScale( &scaleMatrix, bKey->scale.x, bKey->scale.y, bKey->scale.z );
    //     rotationMatrix = MatrixFromQuat( bKey->rotation );
    //     SetTranslation( &translationMatrix, bKey->translation.x, bKey->translation.y, bKey->translation.z );

    //     Mat4 netMatrix = scaleMatrix * rotationMatrix * translationMatrix;
    // //}
}

void InitOrthoCameraMatrix( Mat4* m, float width, float height, float near, float far ) {
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    m->m[0][0] = -1.0f / halfWidth; m->m[0][1] = 0.0f; m->m[0][2] = 0.0f; m->m[0][3] = 0.0f;
    m->m[1][0] = 0.0f; m->m[1][1] = 1.0f / halfHeight; m->m[1][2] = 0.0f; m->m[1][3] = 0.0f;
    m->m[2][0] = 0.0f; m->m[2][1] = 0.0f; m->m[2][2] = -2.0f / (far - near); m->m[2][3] = 0.0f;
    m->m[3][0] = 0.0f; m->m[3][1] = 0.0f; m->m[3][2] = -(far + near) / (far - near); m->m[3][3] = 1.0f;
}

void SetCameraMatrixPosition( Mat4* m, Vec3 p) {
    SetTranslation( m, -p.x * m->m[0][0], -p.y * m->m[1][1], -p.z * m->m[2][2] );
    Mat4 mx; SetToIdentity( &mx );
    Vec3 move = DiffVec( { m->m[3][0], m->m[3][1], m->m[3][2] } , p );
    SetTranslation( &mx, move.x, move.y, move.z );
    *m = (*m * mx);
}

void SetCameraMatrixLookAt( Mat4* m ) {

}

/*-------------------------------------
IMPLEMENTATIONS SPECIFIC TO THE OPEN GL RENDERER
---------------------------------------*/

struct OpenGLTexture {
    GLuint textureID;
    GLenum pixelFormat;
    int width;
    int height;
    GLuint* data;
};

struct ShaderProgram {
    GLuint programID;

    GLint modelMatrixUniformPtr;
    GLint cameraMatrixUniformPtr;

    GLint positionAttribute;
    GLint texCoordAttribute;

    GLint isArmatureAnimatedUniform;
    GLint skeletonUniform;
    GLint boneWeightsAttribute;
    GLint boneIndiciesAttribute;

    uint8_t samplerCount;
    GLint samplerPtrs[4];
};

struct ShaderTextureSet {
    uint8_t count;
    ShaderProgram* associatedShader;
    GLuint shaderSamplerPtrs[4];
    GLuint texturePtrs[4];
};

struct OpenGLMeshBinding {
    GLuint vertexCount;
    GLuint vbo;
    GLuint ibo;
    GLuint uvBuffer;

    Mat4* modelMatrix;

    bool isArmatureAnimated;
    Skeleton* skeleton;
    GLuint boneWeightBuffer;
    GLuint boneIndexBuffer;
};

struct OpenGLFramebuffer {
    GLuint framebufferPtr;
    OpenGLTexture framebufferTexture;
    GLuint framebuffVBO;
    GLuint framebuffIBO;
    GLuint framebuffUVBuff;
};

//void InitRenderer();
//void* CreateRenderableMesh( MeshData* genericMeshData );
//void RenderMesh( void* rendererSpecificMesh );
//void RenderLine( float* lineVertexBuffer, uint16 vertexCount );

#define CLAY_RENDERER
#endif