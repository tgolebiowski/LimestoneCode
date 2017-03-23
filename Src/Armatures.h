#define MAXBONES 32
struct Bone {
	Mat4 bindPose, invBindPose;
    Srt transform;
	Bone* parent;
	Bone* children[4];
	char name[32];
	uint8 childCount;
	uint8 boneIndex;
};

struct Armature {
	Bone bones[ MAXBONES ];
	Bone* rootBone;
	uint8 boneCount;
};

struct BoneKeyFrame {
    Srt transform;
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
        BoneKeyFrame* boneKey = &pose->targetBoneTransforms[ boneIndex ];
        bone->transform = boneKey->transform;
        /* *bone->currentTransform = MultMatrix( 
            bone->invBindPose, 
            pose->targetBoneTransforms[ boneIndex ].combinedMatrix 
        );*/
    }
}

ArmatureKeyFrame BlendKeyFrames( 
    ArmatureKeyFrame* keyframeA, 
    ArmatureKeyFrame* keyframeB, 
    float weight, 
    uint8 boneCount 
) {
    assert( weight >= 0.0f && weight <= 1.0f );

    float keyAWeight, keyBWeight;
    ArmatureKeyFrame out;
    keyAWeight = weight;
    keyBWeight = 1.0f - keyAWeight;

    for( uint8 boneIndex = 0; boneIndex < boneCount; ++boneIndex ) {
        BoneKeyFrame* netBoneKey = &out.targetBoneTransforms[ boneIndex ];
        BoneKeyFrame* bonekeyA = &keyframeA->targetBoneTransforms[ boneIndex ];
        BoneKeyFrame* bonekeyB = &keyframeB->targetBoneTransforms[ boneIndex ];

        netBoneKey->transform.translation = { 
            bonekeyA->transform.translation.x * keyAWeight + bonekeyB->transform.translation.x * keyBWeight,
            bonekeyA->transform.translation.y * keyAWeight + bonekeyB->transform.translation.y * keyBWeight,
            bonekeyA->transform.translation.z * keyAWeight + bonekeyB->transform.translation.z * keyBWeight
        };

        //TODO: logic for non=-axis aligned scale vec
        netBoneKey->transform.scale = {
            bonekeyA->transform.scale.vec.x * keyAWeight + bonekeyB->transform.scale.vec.x * keyBWeight,
            bonekeyA->transform.scale.vec.y * keyAWeight + bonekeyB->transform.scale.vec.y * keyBWeight,
            bonekeyA->transform.scale.vec.z * keyAWeight + bonekeyB->transform.scale.vec.z * keyBWeight
        };

        float q_dot = bonekeyA->transform.rotation.w * bonekeyB->transform.rotation.w +
        bonekeyA->transform.rotation.x * bonekeyB->transform.rotation.x +
        bonekeyA->transform.rotation.y * bonekeyB->transform.rotation.y +
        bonekeyA->transform.rotation.z * bonekeyB->transform.rotation.z;

        if( q_dot < 0.0f ) {
            bonekeyA->transform.rotation.w *= -1.0f;
            bonekeyA->transform.rotation.x *= -1.0f;
            bonekeyA->transform.rotation.y *= -1.0f;
            bonekeyA->transform.rotation.z *= -1.0f;
        }

        #if 1
        //TODO: Slerp Option
        netBoneKey->transform.rotation = {
            bonekeyA->transform.rotation.w * keyAWeight + bonekeyB->transform.rotation.w * keyBWeight,
            bonekeyA->transform.rotation.x * keyAWeight + bonekeyB->transform.rotation.x * keyBWeight,
            bonekeyA->transform.rotation.y * keyAWeight + bonekeyB->transform.rotation.y * keyBWeight,
            bonekeyA->transform.rotation.z * keyAWeight + bonekeyB->transform.rotation.z * keyBWeight
        };
 
        float qNorm = sqrt( netBoneKey->transform.rotation.w * netBoneKey->transform.rotation.w + 
            netBoneKey->transform.rotation.x * netBoneKey->transform.rotation.x + 
            netBoneKey->transform.rotation.y * netBoneKey->transform.rotation.y + 
            netBoneKey->transform.rotation.z * netBoneKey->transform.rotation.z );
        netBoneKey->transform.rotation.w /= qNorm;
        netBoneKey->transform.rotation.x /= qNorm;
        netBoneKey->transform.rotation.y /= qNorm;
        netBoneKey->transform.rotation.z /= qNorm;

        #else 
        netBoneKey->rotation = Slerp( 
            bonekeyA->rotation, 
            bonekeyB->rotation, 
            weight 
        );
        #endif
    }

    return out;
}