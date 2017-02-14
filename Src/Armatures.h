#define MAXBONES 32
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
	Bone bones[ MAXBONES ];
	Mat4 boneTransforms[ MAXBONES ];
	Bone* rootBone;
	uint8 boneCount;
};

struct BoneKeyFrame {
	Mat4 combinedMatrix;
    Quat rotation;
	Vec3 translation, scale;
};

struct ArmatureKeyFrame {
	BoneKeyFrame targetBoneTransforms[ MAXBONES ];
};

static Bone* GetBoneByName( Armature* armature, char* name ) {
    for( int i = 0; i < armature->boneCount; ++i ) {
        Bone* bone = &armature->bones[ i ];
        int compare = strcmp( &bone->name[0], name );
        if( compare == 0 ) {
            return bone;
        }
    }

    return NULL;
}

void ApplyKeyFrameToArmature( ArmatureKeyFrame* pose, Armature* armature ) {
    for( uint8 boneIndex = 0; boneIndex < armature->boneCount; ++boneIndex ) {
        Bone* bone = &armature->bones[ boneIndex ];
        *bone->currentTransform = pose->targetBoneTransforms[ boneIndex ].combinedMatrix;
        /* *bone->currentTransform = MultMatrix( 
            bone->invBindPose, 
            pose->targetBoneTransforms[ boneIndex ].combinedMatrix 
        );*/
    }
}

ArmatureKeyFrame BlendKeyFrames( ArmatureKeyFrame* keyframeA, ArmatureKeyFrame* keyframeB, float weight, uint8 boneCount ) {
    float keyAWeight, keyBWeight;
    ArmatureKeyFrame out;
    keyAWeight = weight;
    keyBWeight = 1.0f - keyAWeight;

    for( uint8 boneIndex = 0; boneIndex < boneCount; ++boneIndex ) {
        BoneKeyFrame* netBoneKey = &out.targetBoneTransforms[ boneIndex ];
        BoneKeyFrame* bonekeyA = &keyframeA->targetBoneTransforms[ boneIndex ];
        BoneKeyFrame* bonekeyB = &keyframeB->targetBoneTransforms[ boneIndex ];

        netBoneKey->translation = { 
            bonekeyA->translation.x * keyAWeight + bonekeyB->translation.x * keyBWeight,
            bonekeyA->translation.y * keyAWeight + bonekeyB->translation.y * keyBWeight,
            bonekeyA->translation.z * keyAWeight + bonekeyB->translation.z * keyBWeight
        };
        netBoneKey->scale = {
            bonekeyA->scale.x * keyAWeight + bonekeyB->scale.x * keyBWeight,
            bonekeyA->scale.y * keyAWeight + bonekeyB->scale.y * keyBWeight,
            bonekeyA->scale.z * keyAWeight + bonekeyB->scale.z * keyBWeight
        };
        //TODO: Slerp Option
        netBoneKey->rotation = {
            bonekeyA->rotation.w * keyAWeight + bonekeyB->rotation.w * keyBWeight,
            bonekeyA->rotation.x * keyAWeight + bonekeyB->rotation.x * keyBWeight,
            bonekeyA->rotation.y * keyAWeight + bonekeyB->rotation.y * keyBWeight,
            bonekeyA->rotation.z * keyAWeight + bonekeyB->rotation.z * keyBWeight
        };

        netBoneKey->combinedMatrix = Mat4FromComponents( netBoneKey->scale, netBoneKey->rotation, netBoneKey->translation );
    }

    return out;
}