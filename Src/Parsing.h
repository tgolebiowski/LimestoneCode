static int TextToNumberConversion( 
    const char* textIn, 
    float* numbersOut, 
    int maxOutCount 
) {
    const char* start = textIn;
    const char* end = start; 
    uint16 index = 0;
    do {
        //Reset start and end
        start = end;

        //Read to the end of this float token
        do {
            end++;
        } while( *end != ' ' && *end != 0 );
        assert( ( end - start ) < 16 );

        //Copy token into this tiny buffer & null terminate
        char buffer [16] = { };
        size_t tokenLength = end - start;
        memcpy( &buffer[0], start, tokenLength );
        buffer[ end - start ] = 0;

        //Read float value
        numbersOut[ index ] = strtof( buffer, NULL );
        index++;
    } while( *end != 0 && index < maxOutCount );
    return index;
}

#include <functional>
#include "tinyxml2/tinyxml2.h"
#include "tinyxml2/tinyxml2.cpp"

//TODO: maybe do Using tinyxml2 namespace?

static Mat4 FlipMatrixOrientation( Mat4 m ) {
    float yAxis [4] = { }; memcpy( yAxis, m[1], sizeof(float) * 4 );
    float zAxis [4] = { }; memcpy( zAxis, m[2], sizeof(float) * 4 );
    zAxis[0] *= -1.0f; zAxis[1] *= -1.0f; zAxis[2] *= -1.0f; zAxis[3] *= -1.0f;
    memcpy( m[1], zAxis, sizeof( float ) * 4 );
    memcpy( m[2], yAxis, sizeof( float ) * 4 );

    return m;
}

//Parsing basic bone data from XML
static Bone* ParseColladaBoneData(
    tinyxml2::XMLElement* boneElement,
    Armature* armature,
    Bone* parentBone
) {
    //Create a new bone object
    Bone* bone = &armature->bones[ armature->boneCount ];
    {
        bone->parent = parentBone;
        bone->boneIndex = armature->boneCount;
        strcpy( &bone->name[0], boneElement->Attribute( "sid" ) );
        armature->boneCount++;
    }

    //Read Matrix character data & convert to float
    Mat4 m = { };
    {
        char matrixTextData [512] = { };
        memset( &matrixTextData, 0, sizeof(char) * 512 );
        strcpy( 
            &matrixTextData[0], 
            boneElement->FirstChildElement("matrix")->FirstChild()->ToText()->Value() 
        );
        TextToNumberConversion( matrixTextData, (float*)&m, 16 );
        m = TransposeMatrix( m );
    }

    //Some logic, if the bone has a parent or not
    if( parentBone == NULL ) {
        armature->rootBone = bone;
        //TODO: check collada for the Z_UP value
        bone->bindPose = FlipMatrixOrientation( m );
    } else {
        bone->bindPose = MultMatrix( m, parentBone->bindPose );
    }
    DecomposeMat4( bone->bindPose, 
        &bone->transform.scale.vec,
        &bone->transform.rotation,
        &bone->transform.translation
    );

    //Iterate for all children
    bone->childCount = 0;
    tinyxml2::XMLElement* childBoneElement = boneElement->FirstChildElement( "node" );
    while( childBoneElement != NULL ) {
        Bone* childBone = ParseColladaBoneData( childBoneElement, armature, bone );
        bone->children[ bone->childCount++ ] = childBone;
        tinyxml2::XMLNode* siblingNode = childBoneElement->NextSibling();
        if( siblingNode != NULL ) {
            childBoneElement = siblingNode->ToElement();
        } else {
            childBoneElement = NULL;
        }
    };

    return bone;
}

//TODO: refactor this method
static bool ParseMeshDataFromCollada( 
    void* rawData, 
    Stack* allocater, 
    MeshGeometryData* storage, 
    Armature* armature 
) {
    if( rawData == NULL ) {
        printf( "No data passed in.\n" );
        return false;
    }

    tinyxml2::XMLDocument colladaDoc;
    colladaDoc.Parse( (const char*)rawData );

    tinyxml2::XMLElement* colladaNode = colladaDoc.FirstChildElement( "COLLADA" );
    tinyxml2::XMLElement* meshNode = colladaNode
        ->FirstChildElement( "library_geometries" )
        ->FirstChildElement( "geometry" )
        ->FirstChildElement( "mesh" );

    //char* colladaTextBuffer = NULL;
    size_t textBufferLen = 0;

    uint16 vCount = 0;
    uint16 nCount = 0;
    uint16 uvCount = 0;
    uint16 indexCount = 0;
    float* rawColladaVertexData;
    float* rawColladaNormalData;
    float* rawColladaUVData = NULL;
    float* rawIndexData;
    ///Basic Mesh Geometry Data
    {
        tinyxml2::XMLNode* colladaVertexArray = meshNode->FirstChildElement( "source" );
        tinyxml2::XMLElement* vertexFloatArray = colladaVertexArray->FirstChildElement( "float_array" );
        tinyxml2::XMLNode* colladaNormalArray = colladaVertexArray->NextSibling();
        tinyxml2::XMLElement* normalFloatArray = colladaNormalArray->FirstChildElement( "float_array" );
        tinyxml2::XMLNode* colladaUVMapArray = colladaNormalArray->NextSibling();
        tinyxml2::XMLElement* uvMapFloatArray = colladaUVMapArray->FirstChildElement( "float_array" );
        tinyxml2::XMLElement* meshSrc = meshNode->FirstChildElement( "polylist" );
        tinyxml2::XMLElement* colladaIndexArray = meshSrc->FirstChildElement( "p" );

        int dataInputCount = 0;
        int count;
        const char* colladaVertArrayVal = vertexFloatArray->FirstChild()->Value();
        vertexFloatArray->QueryAttribute( "count", &count );
        vCount = count;
        rawColladaVertexData = (float*)malloc( sizeof(float) * vCount );
        dataInputCount++;

        const char* colladaNormArrayVal = normalFloatArray->FirstChild()->Value();
        normalFloatArray->QueryAttribute( "count", &count );
        nCount = count;
        rawColladaNormalData = (float*)malloc( sizeof(float) * nCount );
        dataInputCount++;

        char* colladaUVMapArrayVal = NULL;
        if( uvMapFloatArray != NULL ) {
            colladaUVMapArrayVal = (char*)uvMapFloatArray->FirstChild()->Value();
            uvMapFloatArray->QueryAttribute( "count", &count );
            uvCount = count;
            rawColladaUVData = (float*)malloc( sizeof(float) * uvCount );
            dataInputCount++;
        }

        const char* colladaIndexArrayVal = colladaIndexArray->FirstChild()->Value();
        meshSrc->QueryAttribute( "count", &count );
        //Assume this is already triangulated
        indexCount = count * 3 * dataInputCount;
        rawIndexData = (float*)malloc( sizeof(float) * indexCount );

        memset( rawColladaVertexData, 0, sizeof(float) * vCount );
        memset( rawColladaNormalData, 0, sizeof(float) * nCount );
        if(rawColladaUVData != NULL ) {
            memset( rawColladaUVData, 0, sizeof(float) * uvCount );
        }
        memset( rawIndexData, 0, sizeof(float) * indexCount );

        //Reading Vertex position data
        int vertsParsed = TextToNumberConversion( 
            colladaVertArrayVal, 
            rawColladaVertexData,
            vCount
        );
        assert( vCount == vertsParsed );

        //Reading Normals data
        int normalsParsed = TextToNumberConversion( 
            colladaNormArrayVal, 
            rawColladaNormalData,
            nCount
        );
        assert( nCount == normalsParsed );

        //Reading UV map data
        if( colladaUVMapArrayVal != NULL ) {
            int uvsParsed = TextToNumberConversion( 
                colladaUVMapArrayVal, 
                rawColladaUVData,
                uvCount
            );
            assert( uvsParsed = uvCount );
        }

        //Reading index data
        int indiciesParsed = TextToNumberConversion( 
            colladaIndexArrayVal, 
            rawIndexData,
            indexCount
        );
        assert( indiciesParsed == indexCount );
    }

    float* rawBoneWeightData = NULL;
    float* rawBoneIndexData = NULL;
    //Skinning Data
    {
        tinyxml2::XMLElement* libControllers = colladaNode->FirstChildElement( "library_controllers" );
        if( libControllers == NULL ) goto skinningExit;
        tinyxml2::XMLElement* controllerElement = libControllers->FirstChildElement( "controller" );
        if( controllerElement == NULL ) goto skinningExit;

        tinyxml2::XMLNode* vertexWeightDataArray = controllerElement->FirstChild()->FirstChild()->NextSibling()->NextSibling()->NextSibling();
        tinyxml2::XMLNode* vertexBoneIndexDataArray = vertexWeightDataArray->NextSibling()->NextSibling();
        tinyxml2::XMLNode* vCountArray = vertexBoneIndexDataArray->FirstChildElement( "vcount" );
        tinyxml2::XMLNode* vArray = vertexBoneIndexDataArray->FirstChildElement( "v" );
        const char* boneWeightsData = vertexWeightDataArray->FirstChild()->FirstChild()->Value();
        const char* vCountArrayData = vCountArray->FirstChild()->Value();
        const char* vArrayData = vArray->FirstChild()->Value();

        float* colladaBoneWeightData = NULL;
        float* colladaBoneIndexData = NULL;
        float* colladaBoneInfluenceCounts = NULL;
        ///This is overkill, Collada stores ways less data usually, plus this still doesn't account for very complex models 
        ///(e.g, lots of verts with more than MAXBONESPERVERT influencing position )
        colladaBoneWeightData = (float*)malloc( sizeof(float) * MAXBONESPERVERT * vCount );
        colladaBoneIndexData = (float*)malloc( sizeof(float) * MAXBONESPERVERT * vCount );
        colladaBoneInfluenceCounts = (float*)malloc( sizeof(float) * MAXBONESPERVERT * vCount );

        //Read bone weights data
        TextToNumberConversion( 
            boneWeightsData, 
            colladaBoneWeightData, 
            MAXBONESPERVERT * vCount 
        );

        //Read bone index data
        TextToNumberConversion( 
            vArrayData, 
            colladaBoneIndexData, 
            MAXBONESPERVERT * vCount 
        );

        //Read bone influence counts
        TextToNumberConversion( 
            vCountArrayData, 
            colladaBoneInfluenceCounts, 
            MAXBONESPERVERT * vCount 
        );

        rawBoneWeightData = (float*)malloc( sizeof(float) * MAXBONESPERVERT * vCount );
        rawBoneIndexData = (float*)malloc( sizeof(float) * MAXBONESPERVERT * vCount );
        memset( rawBoneWeightData, 0, sizeof(float) * MAXBONESPERVERT * vCount );
        memset( rawBoneIndexData, 0, sizeof(float) * MAXBONESPERVERT * vCount );

        int colladaIndexIndirection = 0;
        int verticiesInfluenced = 0;
        vCountArray->Parent()->ToElement()->QueryAttribute( "count", &verticiesInfluenced );
        for( uint16 i = 0; i < verticiesInfluenced; i++ ) {
            uint8 influenceCount = colladaBoneInfluenceCounts[i];
            for( uint16 j = 0; j < influenceCount; j++ ) {
                uint16 boneIndex = colladaBoneIndexData[ colladaIndexIndirection++ ];
                uint16 weightIndex = colladaBoneIndexData[ colladaIndexIndirection++ ];
                rawBoneWeightData[ i * MAXBONESPERVERT + j ] = colladaBoneWeightData[ weightIndex ];
                rawBoneIndexData[ i * MAXBONESPERVERT + j ] = boneIndex;
            }
        }

        free( colladaBoneWeightData );
        free( colladaBoneIndexData );
        free( colladaBoneInfluenceCounts );
        free( rawBoneWeightData );
        free( rawBoneIndexData );
    }
    skinningExit:

    //output to my version of storage
    storage->dataCount = 0;
    uint16 counter = 0;

    const uint32 vertCount = indexCount / 3;
    storage->vData = (Vec3*)StackAllocAligned( allocater, vertCount * sizeof( Vec3 ), 16 );
    storage->uvData = (Vec2*)StackAllocAligned( allocater, vertCount * sizeof( Vec2 ), 4 );
    storage->normalData = (Vec3*)StackAllocAligned( allocater, vertCount * sizeof( Vec3 ), 4 );
    if( rawBoneWeightData != NULL ) {
        storage->boneWeightData = (float*)StackAllocAligned( allocater, sizeof(float) * vertCount * MAXBONESPERVERT, 4 );
        storage->boneIndexData = (uint32*)StackAllocAligned( allocater, sizeof(uint32) * vertCount * MAXBONESPERVERT, 4 );
    } else {
        storage->boneWeightData = NULL;
        storage->boneIndexData = NULL;
    }

    while( counter < indexCount ) {
        Vec3 v, n;
        Vec2 uv;

        uint16 vertIndex = rawIndexData[ counter++ ];
        uint16 normalIndex = rawIndexData[ counter++ ];

        uint16 uvIndex = 0;
        if( rawColladaUVData != NULL ) {
            uvIndex = rawIndexData[ counter++ ];
        }

        v.x = rawColladaVertexData[ vertIndex * 3 + 0 ];
        v.z = -rawColladaVertexData[ vertIndex * 3 + 1 ];
        v.y = rawColladaVertexData[ vertIndex * 3 + 2 ];

        n.x = rawColladaNormalData[ normalIndex * 3 + 0 ];
        n.z = -rawColladaNormalData[ normalIndex * 3 + 1 ];
        n.y = rawColladaNormalData[ normalIndex * 3 + 2 ];

        if( rawColladaUVData != NULL ) {
            //NOTE:OpenGL has its 1,1 corner in the top right, but also uploads textures
            //upside down(??or is that just stbimage?), so this is fine :I
            uv.x = rawColladaUVData[ uvIndex * 2 ];
            uv.y = 1.0f - rawColladaUVData[ uvIndex * 2 + 1 ];
        } else {
            uv = { 0.0f, 0.0f };
        }

        //TODO: need index for "new" verticies, since dataCount will fall out of sync
        //as soon as there is one copy.
        //TODO: add and normalize normals, simple but will fail on pointy bits
        //TODO: write index to storage->index[ dataCount ]
        //TODO IN RENDERER: setup and bind index buffer
        //TODO IN RENDERER: support for imm mode verts, w/ no index buffs

        if( storage->dataCount == 0 ) {
            storage->aabbMin = v;
            storage->aabbMax = v;
        } else {
            storage->aabbMin = {
                MINf( v.x, storage->aabbMin.x ),
                MINf( v.y, storage->aabbMin.y ),
                MINf( v.z, storage->aabbMin.z )
            };

            storage->aabbMax = {
                MAXf( v.x, storage->aabbMax.x ),
                MAXf( v.y, storage->aabbMax.y ),
                MAXf( v.z, storage->aabbMax.z )
            };
        }

        uint32 storageIndex = storage->dataCount;
        storage->vData[ storageIndex ] = v;
        storage->normalData[ storageIndex ] = n;
        if( rawColladaUVData != NULL ) {
            storage->uvData[ storageIndex ] = uv;
        }

        if( rawBoneWeightData != NULL ) {
            uint16 boneDataIndex = storage->dataCount * MAXBONESPERVERT;
            uint16 boneVertexIndex = vertIndex * MAXBONESPERVERT;
            for( uint8 i = 0; i < MAXBONESPERVERT; i++ ) {
                storage->boneWeightData[ boneDataIndex + i ] = rawBoneWeightData[ boneVertexIndex + i ];
                storage->boneIndexData[ boneDataIndex + i ] = rawBoneIndexData[ boneVertexIndex + i ];
            }
        }
        storage->dataCount++;
    };

    free( rawColladaVertexData );
    free( rawColladaNormalData );
    if( rawColladaUVData != NULL ) { free( rawColladaUVData ); }
    free( rawIndexData );

    return true;
}

static bool ParseArmatureDataFromCollada( 
    void* data, 
    Stack* allocater, 
    Armature* armature 
) {
    if( data == NULL ) {
        printf( "No data passed in.\n" );
        return false;
    }

    tinyxml2::XMLDocument colladaDoc;
    colladaDoc.Parse( (const char*)data );

    //Get the type of element that could store an armature
    tinyxml2::XMLElement* colladaElement = colladaDoc.FirstChildElement( "COLLADA" );
    tinyxml2::XMLElement* visualScenesNode = NULL;
    {
        //TODO: check for nulls at each step
        visualScenesNode = colladaElement
        ->FirstChildElement( "library_visual_scenes" )
        ->FirstChildElement( "visual_scene" )
        ->FirstChildElement( "node" );
    }

    //Step through scene heirarchy until start of armature is found
    tinyxml2::XMLElement* armatureNode = NULL;
    while( visualScenesNode != NULL ) {
        if( visualScenesNode->FirstChildElement( "node" ) != NULL && 
            visualScenesNode->FirstChildElement( "node" )->Attribute( "type", "JOINT" ) != NULL 
        ) {
            armatureNode = visualScenesNode;
            break;
        } else {
            visualScenesNode = visualScenesNode->NextSibling()->ToElement();
        }
    }
    if( armatureNode == NULL ) {
        printf( "No armature data found in this file\n" );
        return false;
    }

    //Parse bind poses, number bones etc. recursively
    armature->boneCount = 0;
    tinyxml2::XMLElement* boneElement = armatureNode->FirstChildElement( "node" );
    ParseColladaBoneData( boneElement, armature, NULL );

    //TODO: clean/refactor this mess
    //Parse inverse bind pose data from skinning section of XML
    tinyxml2::XMLElement* controllersElement = colladaElement
        ->FirstChildElement( "library_controllers" )
        ->FirstChildElement( "controller" );
    if( controllersElement != NULL ) {
        tinyxml2::XMLElement* boneNamesSource = controllersElement
            ->FirstChildElement( "skin" )
            ->FirstChildElement( "source" );
        tinyxml2::XMLElement* boneBindPoseSource = boneNamesSource->NextSibling()->ToElement();

        float* boneMatriciesData = (float*)malloc( sizeof(float) * 16 * armature->boneCount );
        const char* boneNameArrayData = boneNamesSource->FirstChild()->FirstChild()->Value();
        const char* boneMatrixTextData = boneBindPoseSource->FirstChild()->FirstChild()->Value();
        size_t nameDataLen = strlen( boneNameArrayData );
        size_t matrixDataLen = strlen( boneMatrixTextData );

        char* boneNamesLocalCopy = NULL;
        boneNamesLocalCopy = (char*)malloc( nameDataLen + 1 );
        memset( boneNamesLocalCopy, 0, nameDataLen + 1 );

        char* colladaTextBuffer = NULL;
        colladaTextBuffer = (char*)malloc( matrixDataLen );

        memcpy( boneNamesLocalCopy, boneNameArrayData, nameDataLen );
        memcpy( colladaTextBuffer, boneMatrixTextData, matrixDataLen );
        TextToNumberConversion( colladaTextBuffer, boneMatriciesData, 16 );
        char* nextBoneName = &boneNamesLocalCopy[0];
        for( uint8 matrixIndex = 0; matrixIndex < armature->boneCount; matrixIndex++ ) {
            Mat4 matrix;
            memcpy( &matrix.m[0], &boneMatriciesData[matrixIndex * 16], sizeof(float) * 16 );

            char boneName [32];
            char* boneNameEnd = nextBoneName;
            do {
                boneNameEnd++;
            } while( *boneNameEnd != ' ' && *boneNameEnd != 0 );
            size_t charCount = boneNameEnd - nextBoneName;
            memset( boneName, 0, sizeof( char ) * 32 );
            memcpy( boneName, nextBoneName, charCount );
            nextBoneName = boneNameEnd + 1;

            Bone* targetBone = NULL;
            for( uint8 boneIndex = 0; boneIndex < armature->boneCount; boneIndex++ ) {
                Bone* bone = &armature->bones[ boneIndex ];
                if( strcmp( bone->name, boneName ) == 0 ) {
                    targetBone = bone;
                    break;
                }
            }

            targetBone->invBindPose = TransposeMatrix( matrix );
            targetBone->invBindPose = FlipMatrixOrientation( targetBone->invBindPose );
        }

        free( boneMatriciesData );
        free( boneNamesLocalCopy );
        free( colladaTextBuffer );
    } else {
        for( int i = 0; i < armature->boneCount; ++i ) {
            Bone* bone = &armature->bones[ i ];
            bone->invBindPose = InverseMatrix( bone->bindPose );
        }
    }
}

static void LoadAnimationDataFromCollada( 
    const char* fileName, 
    ArmatureKeyFrame* keyframe, 
    Armature* armature 
) {
    tinyxml2::XMLDocument colladaDoc;
    colladaDoc.LoadFile( fileName );

    tinyxml2::XMLNode* animationNode = colladaDoc.FirstChildElement( "COLLADA" )
        ->FirstChildElement( "library_animations" )
        ->FirstChild();

    Mat4* localTransforms = NULL;
    localTransforms = (Mat4*)malloc( sizeof( Mat4 ) * armature->boneCount );
    assert( localTransforms );
    memset( localTransforms, 0, sizeof( Mat4 ) * armature->boneCount );

    //Read each local transform matrix
    while( animationNode != NULL ) {
        //Desired data: what bone, and what local transform to it occurs
        Mat4 boneLocalTransform = { };
        Bone* targetBone = NULL;

        //Parse the target attribute from the XMLElement for channel, and get the bone it corresponds to
        const char* transformName = animationNode
            ->FirstChildElement( "channel" )
            ->Attribute( "target" );
        char boneNameDelimiter [1] = { '/' };
        size_t targetBoneNameLen = strcspn( transformName, boneNameDelimiter );

        char* targetBoneName = (char*)malloc( targetBoneNameLen + 1 );
        memset( targetBoneName, 0, targetBoneNameLen + 1 );
        memcpy( targetBoneName, transformName, targetBoneNameLen );
        for( uint8 boneIndex = 0; boneIndex < armature->boneCount; boneIndex++ ) {
            if( strcmp( armature->bones[ boneIndex ].name, targetBoneName ) == 0 ) {
                targetBone = &armature->bones[ boneIndex ];
                break;
            }
        }

        //get the local bone transform matrix
        {
            //Parse matrix data, and extract first keyframe data
            tinyxml2::XMLNode* transformMatrixElement = animationNode
                ->FirstChild()
                ->NextSibling();
            const char* matrixTransformData = transformMatrixElement
                ->FirstChild()
                ->FirstChild()
                ->Value();
            size_t transformDataLen = strlen( matrixTransformData ) + 1;
            char* transformDataCopy = (char*)malloc( transformDataLen * sizeof( char ) );
            memset( transformDataCopy, 0, transformDataLen );
            memcpy( transformDataCopy, matrixTransformData, transformDataLen );

            float rawTransformData [16] = { };
            int count = 0; 
            transformMatrixElement->FirstChildElement()->QueryAttribute( "count", &count );
            assert( count == 16 );
            memset( rawTransformData, 0, 16 * sizeof(float) );
            TextToNumberConversion( transformDataCopy, rawTransformData, count );
            memcpy( &boneLocalTransform.m[0][0], &rawTransformData[0], 16 * sizeof(float) );

            free( transformDataCopy );
        }

        boneLocalTransform = TransposeMatrix( boneLocalTransform );
        if( targetBone == armature->rootBone ) {
            boneLocalTransform = FlipMatrixOrientation( boneLocalTransform );
        }
        localTransforms[ targetBone->boneIndex ] = boneLocalTransform;

        animationNode = animationNode->NextSibling();

        free( targetBoneName );
    }

    //Pre multiply bones with parents to save doing it during runtime
    struct {
        ArmatureKeyFrame* keyframe;
        Mat4* localTransforms;
        void PremultiplyKeyFrame( Bone* target, Mat4 parentTransform ) {
            BoneKeyFrame* boneKey = &keyframe->targetBoneTransforms[ target->boneIndex ];
            Mat4 localMatrix = localTransforms[ target->boneIndex ];
            Mat4 netMatrix = MultMatrix( localMatrix, parentTransform );

            for( uint8 boneIndex = 0; boneIndex < target->childCount; boneIndex++ ) {
                PremultiplyKeyFrame( target->children[ boneIndex ], netMatrix );
            }
            DecomposeMat4( netMatrix, 
                &boneKey->transform.scale.vec, 
                &boneKey->transform.rotation, 
                &boneKey->transform.translation 
            );
        }
    }LocalRecursiveScope;
    LocalRecursiveScope.keyframe = keyframe;
    LocalRecursiveScope.localTransforms = localTransforms;
    LocalRecursiveScope.PremultiplyKeyFrame( armature->rootBone, IdentityMatrix );

    free( localTransforms );
}