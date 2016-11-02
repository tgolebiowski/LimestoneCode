#define DLL_ONLY
#include "App.h"
#include "DebugDraw.h"
#include "DearImGui_Limestone.h"

#include <cstdlib>

//Over-Engineering TODOs:
/*
* Make a struct for previous Frame Data
* Consolidate player state into list of bools, explicitly update these
*/

static void MakeIcosphere( MeshGeometryData* data, Stack* allocater ) {
	float t = ( 1.0f + sqrt( 5.0f ) ) / 2.0f;

	Vec3 basePoints [12] = {
		{ -1.0f, t, 0.0f },
		{ 1.0f, t, 0.0f },
		{ -1.0f, -t, 0.0f },
		{ 1.0f, -t, 0.0f },

		{ 0.0f, -1.0f, t },
		{ 0.0f, 1.0f, t },
		{ 0.0f, -1.0f, -t },
		{ 0.0f, 1.0f, -t },

		{ t, 0.0f, -1.0f },
		{ t, 0.0f, 1.0f },
		{ -t, 0.0f, -1.0f },
		{ -t, 0.0f, 1.0f }
	};

	float radius = 0.5f;
	for( int i = 0; i < 12; ++i ) {
		Normalize( &basePoints[ i ] );
		basePoints[i] = basePoints[ i ] * radius;
	}

	int indexList [ 20 * 3 ] = {
		0, 11, 5,
		0, 5, 1,
		0, 1, 7,
		0, 7, 10,
		0, 10, 11,

		1, 5, 9,
		5, 11, 4,
		11, 10, 2,
		10, 7, 6,
		7, 1, 8,

		3, 9, 4,
		3, 4, 2,
		3, 2, 6,
		3, 6, 8,
		3, 8, 9,

		4, 9, 5,
		2, 4, 11,
		6, 2, 10,
		8, 6, 7,
		9, 8, 1
	};

	data->vData = (Vec3*)StackAllocAligned( allocater, sizeof( Vec3 ) * 20 * 3 );

	for( int i = 0; i < 20 * 3; ++i ) {
		data->vData[ i ] = basePoints[ indexList[ i ] ];
	}

	data->dataCount = 20 * 3;
}

static float DistanceFromLineSeg( Vec3 p, Vec3 wall_p1, Vec3 wall_p2, Vec3* normalAway ) {
	Vec3 wallChange = wall_p2 - wall_p1;
	Vec3 wall_p1ToP = p - wall_p1;
	Vec3 wall_p2ToP = p - wall_p2;
	if( Dot( wallChange, wall_p1ToP ) < 0.0f ) {
		*normalAway = { wall_p1ToP.x, 0.0f, wall_p1ToP.y };
		Normalize( normalAway );
		return Vec3Length( wall_p1ToP );
	} 
	if( Dot( wallChange * -1.0f, wall_p2ToP ) < 0.0f ) {
		*normalAway = wall_p2ToP;
		Normalize( normalAway );
		return Vec3Length( wall_p2ToP );
	}

	float dist = std::abs( wallChange.z * p.x - wallChange.x * p.z  + wall_p2.x * wall_p1.z - wall_p2.z * wall_p1.x ) /
	sqrtf( wallChange.z * wallChange.z + wallChange.x * wallChange.x );

	*normalAway = { -wallChange.z, 0.0f, wallChange.x };
	if( Dot( *normalAway, wall_p1ToP ) < 0.0f ) {
		*normalAway = { -normalAway->x, 0.0f, -normalAway->z };
	}
	Normalize( normalAway );

	return dist;
}

//Global consts
Vec3 roomDimensions = { 12.0f, 0.0f, 12.0f };
Vec3 forwardVec = { 0.0f, 0.0f, 1.0f };

struct WindState {
	Vec3 dir;
	float intensity;
};

struct ParticleState {
	Vec3 p;
	float t;
};

struct ParticleGroup {
	ParticleState* pArray;
	int particleCount;
};

struct Cloud {
	Vec3 p;
	Vec3 aabb;

	int borderCount;
	Vec3* borders;

	ParticleGroup* particles;
};

struct Room {
	struct Wall {
		Vec3* points;
		int pointCount;	
	};
	Wall* walls;
	int wallCount;

	Cloud* cloud;

	int xIndex;
	int yIndex;
	int zIndex;
};

inline static bool IsInsideShape( Vec3 p, Vec3* b, int bCount ) {
	int leftIntersection = 0;
	int rightIntersection = 0;
	for( int i = 0; i < bCount; ++i ) {
		Vec3 s1 = b[ i ];
		Vec3 s2 = b[ ( i + 1 ) % bCount ];

		if( ( s1.z >= p.z && s2.z <= p.z ) || ( s1.z <= p.z && s2.z >= p.z ) ) {
			float yDelta = s2.z - s1.z;
			float d = s2.z - p.z;
			if( abs( yDelta ) < 1e-06 ) { continue; }
			float t = d / yDelta;

			float x = ( ( s2.x - s1.x ) * t ) + s1.x;

			leftIntersection += ( x < p.x ) * 1;
			rightIntersection += ( x > p.x ) * 1;
		}
	}

	return ( ( leftIntersection % 2 ) == 1 && ( rightIntersection % 2 ) == 1 );
}

inline static void UpdateAndRenderRoom( Room* room, Vec3 offset,
	PrimitiveDrawingData* primRenderer,
	Mat4* viewProjectionMat,
	float millisecondsElapsed,
	float* levelOfBadness,
	float* amountOfResource,
	float* soundEffectVolume,
	Vec3 playerLocalP,
	bool hasBubble
) {
	for( int j = 0; j < room->wallCount; ++j ) {
		Room::Wall wall = room->walls[j];

		for(int i = 0; i < wall.pointCount - 1; ++i ) {
			const Vec3 heightVec = { 0.0f, 0.5f, 0.0f };

			Vec3 b1 = wall.points[ i ] + offset;
			Vec3 b2 = wall.points[ i + 1 ] + offset;

			Vec3 tris [6] = { 
				b2, b1, b1 + heightVec,
				b2, b2 + heightVec, b1 + heightVec
			};

			float color [4] = { 1.0f, 1.0f, 1.0f, 1.0f };

			DrawVertsImmediate(
				primRenderer,
				(Vec3*)tris,
				6,
				false,
				true,
				viewProjectionMat,
				NULL,
				(float*)color
			);
		}
	}

	if( room->cloud != NULL ) {
		//Update & Render particles
		ParticleGroup* cloudParticles = room->cloud->particles;
		{
			float loopLength = 1.0f * 1000.0f; //1000 milliseconds, 1 millisecond
			float step = millisecondsElapsed / loopLength;
			for( int i = 0; i < cloudParticles->particleCount; ++i ) {
				cloudParticles->pArray[ i ].t += step;
				if( cloudParticles->pArray[ i ].t > 1.0f ) {
					cloudParticles->pArray[ i ].t -= 
					    (float)( (int)cloudParticles->pArray[i].t );
				}

				float rise = 1.0f;
				float len1 = 0.33f;
				float len2 = 0.75f;

				float t = cloudParticles->pArray[ i ].t;

				float baseColor [4] = { 0.56, 0.22, 0.73, 0.8 };

				Vec3 p1 = cloudParticles->pArray[ i ].p;

				p1.x *= room->cloud->aabb.x;
				p1.y += rise * t;
				p1.z *= room->cloud->aabb.z;

				p1 = p1 + room->cloud->p;

				bool pointIsInBorder = IsInsideShape( 
					p1, 
					room->cloud->borders, 
					room->cloud->borderCount
				);
				if( !pointIsInBorder ) {
					continue;
				}

				p1 = p1 + offset;

				baseColor[3] *= ( 1.0f - t );

				float startRad = 0.15f;
				float endRad = 0.25f;
				float rad = ( startRad * ( 1.0f - t ) ) + ( endRad * t );
				DrawDot( 
					primRenderer, 
					p1, 
					rad, 
					viewProjectionMat, 
					(float*)baseColor 
				);
			}
		}

		bool playerIsInHazard = IsInsideShape( 
			playerLocalP + offset,
			room->cloud->borders,
			room->cloud->borderCount
		);

		if( playerIsInHazard ) {
			float dps = 8.0f;
			float damageThisFrame = ( dps * millisecondsElapsed ) / 1000.0f;

			float formulaScaling = 0.1875f;
			float damageNegationFactor = 1.0f - ( atan( *levelOfBadness * formulaScaling )
				/ ( PI * 0.5f ) );
			if( damageNegationFactor < 0 ) damageNegationFactor = 0.0f;
			float netDamage = damageThisFrame * damageNegationFactor;

			*levelOfBadness += netDamage;
			*amountOfResource -= netDamage;

			*soundEffectVolume = 1.0f;
		} else {
			float fullDistVolume = MAXf( room->cloud->aabb.x, room->cloud->aabb.z );
			float minDist = fullDistVolume * 1.5f;

			float dist = Vec3Length( ( room->cloud->p + offset ) - playerLocalP );

			float t = ( minDist - dist ) / ( minDist - fullDistVolume );
			t *= (float)( dist < minDist );

			*soundEffectVolume = t;
		}
		ImGui::Text( "Volume: %.4f", *soundEffectVolume );
	}
}

struct GameMemory {
	Stack frameAllocater;
	Stack imguiDebugMem;
	Stack persistentMem;

	PrimitiveDrawingData primRenderer;

	void* imguiMem;

	//Gameplay related stuff
	float levelOfBadness;
	float amountOfResource;
	Vec3 playerP;

	float bubbleRemaining;

	bool pLastFrame;

	int roomIndex;

	float windState;

	//Game representation stuff
	MeshGeometryData playerDebugMesh;
	float prevFacingAngle;
	float colliderRadius;
	float meshScaling;

	//World representation
	ParticleGroup cloudParticles;

	//General models
	PtrToGpuMem vertexArray;
	PtrToGpuMem normalArray;
	uint32 vertexCount;
	ShaderProgram shader;

	PtrToGpuMem sphereVArray;
	PtrToGpuMem sphereNArray;

	//Sound stuff
	LoadedSound badnessSound;
	PlayingSound* badnessPlaying;
};

extern "C" GAME_INIT( GameInit ) {
    Stack lostSection = AllocateNewStackFromStack( mainSlab, sizeof( GameMemory ) + 32 );
    void* gMemPtr = StackAllocAligned( &lostSection, sizeof( GameMemory ) );
    GameMemory* gMem = (GameMemory*)gMemPtr;

    gMem->frameAllocater = AllocateNewStackFromStack( mainSlab, KILOBYTES( 128 ) );
    gMem->persistentMem = AllocateNewStackFromStack( mainSlab, MEGABYTES( 2 ) );
    gMem->imguiDebugMem = AllocateNewStackFromStack( mainSlab, MEGABYTES( 5 ) );

    InitPrimitveRenderData( &gMem->primRenderer, renderDriver );

    gMem->imguiMem = InitImGui_LimeStone(
    	&gMem->imguiDebugMem,
    	renderDriver,
    	system->windowWidth,
    	system->windowHeight
    );

    void* meshData = system->ReadWholeFile( 
    	"Data/Beakface_Piece.dae", 
    	&gMem->frameAllocater 
    );
    bool parseSucceeded = renderDriver->ParseMeshDataFromCollada( 
    	meshData, 
    	&gMem->frameAllocater, 
    	&gMem->playerDebugMesh, 
    	NULL 
    );

    gMem->vertexArray = renderDriver->CopyVertexDataToGpuMem( 
    	gMem->playerDebugMesh.vData, 
    	sizeof(Vec3) * gMem->playerDebugMesh.dataCount 
    );
    gMem->normalArray = renderDriver->CopyVertexDataToGpuMem( 
    	gMem->playerDebugMesh.normalData, 
    	sizeof(Vec3) * gMem->playerDebugMesh.dataCount 
    );
    gMem->vertexCount = gMem->playerDebugMesh.dataCount;

    CreateShaderProgramFromSourceFiles(
    	renderDriver,
    	"Data/Vert.vert",
    	"Data/Frag.frag",
    	&gMem->shader,
    	&gMem->frameAllocater,
    	system
    );

    gMem->sphereNArray = renderDriver->AllocNewGpuArray();
    gMem->sphereVArray = renderDriver->AllocNewGpuArray();

    ClearStack( &gMem->frameAllocater );

    gMem->playerP = { };
    gMem->levelOfBadness = 0.0f;
    gMem->amountOfResource = 100.0f;
    gMem->bubbleRemaining = 0.0f;
    gMem->pLastFrame = false;

    gMem->roomIndex = 0;

    gMem->colliderRadius = 1.0f;
    gMem->meshScaling = 1.0f;
    gMem->prevFacingAngle = 0.0f;

    //Init debug particle effect
    {
    	gMem->cloudParticles.particleCount = 96;
    	gMem->cloudParticles.pArray = (ParticleState*)StackAllocAligned( 
    		&gMem->persistentMem, 
    		sizeof( ParticleState ) * gMem->cloudParticles.particleCount 
    	);
    	for( int i = 0; i < gMem->cloudParticles.particleCount; ++i ) {
    		ParticleState* b = &gMem->cloudParticles.pArray[ i ];

    		b->t = ((float)( rand() % 100 ) ) / 100.0f;
    		b->p = { 
    			((float)( rand() % 100 ) - 50.0f ) / 50.0f,
    			0.0f,
    			((float)( rand() % 100 ) - 50.0f ) / 50.0f 
    		};
    	}
	}

	gMem->badnessSound = soundDriver->LoadWaveFile( 
		"Data/badness.wav", 
		&gMem->persistentMem 
	);
	gMem->badnessPlaying = QueueLoadedSound(
		&gMem->badnessSound,
		soundDriver->activeSoundList
	);
	gMem->badnessPlaying->playing = true;
	gMem->badnessPlaying->looping = true;
	gMem->badnessPlaying->volume = 0.0f;

    return gMem;
}

extern "C" GAME_UPDATEANDRENDER( UpdateAndRender ) {
	GameMemory* gMem = (GameMemory*)gameMemory;

	if( millisecondsElapsed > 1000.0f / 30.0f ) {
		millisecondsElapsed = 1000.0f / 30.0f;
	}

	UpdateImgui( i, gMem->imguiMem, system->windowWidth, system->windowHeight );

	//Hard code the debug rooms
	Vec3* cloudBorderPoints = (Vec3*)StackAllocAligned( 
		&gMem->frameAllocater, 
		sizeof( Vec3 ) * 12 
	);
	Cloud cloud = {
		{ 0.0f, 0.0f, 0.0f },
		roomDimensions * 0.5f,
		12,
		cloudBorderPoints,
		&gMem->cloudParticles
	};

	WindState windState = {
		{ -1.0f, 0.0f, 0.0f },
		1.0f
	};

	for( int i = 0; i < 12; ++i ) {
		float hazardRad = 5.0f;
		float theta = ( PI * ( 2.0f / 12.0f ) ) * ( (float)i );
		cloud.borders[ i ] = { cosf( theta ) * hazardRad, 0.0f, sinf( theta ) * hazardRad };
		cloud.aabb.x = MAXf( abs( cloud.borders[i].x ), cloud.aabb.x );
		cloud.aabb.y = MAXf( abs( cloud.borders[i].y ), cloud.aabb.y );
		cloud.aabb.z = MAXf( abs( cloud.borders[i].z ), cloud.aabb.z );
	}

	int roomCount = 4;
	Room rooms [4][4] = { };
	for( int i = 0; i < roomCount; ++i ) {
		Room* room = &rooms[0][i];

		room->xIndex = 0;
		room->zIndex = i;

		room->wallCount = 2;
		room->walls = (Room::Wall*)StackAllocAligned(
			&gMem->frameAllocater,
			sizeof( Room::Wall ) * room->wallCount
		);

		//Right side borders
		room->walls[0].pointCount = 4;
		room->walls[0].points = (Vec3*)StackAllocAligned( 
			&gMem->frameAllocater, 
			sizeof( Vec3 ) * 4 
		);
		room->walls[0].points[0] = { 2.0f, 0.0f, -6.0f };
		room->walls[0].points[1] = { 5.85f, 0.0f, -6.0f };
		room->walls[0].points[2] = { 6.0f, 0.0f, 6.0f };
		room->walls[0].points[3] = { 2.0f, 0.0f, 6.0f };

		//Left side borders
		room->walls[1].pointCount = 4;
		room->walls[1].points = (Vec3*)StackAllocAligned( 
			&gMem->frameAllocater, 
			sizeof( Vec3 ) * 4 
		);
		room->walls[1].points[0] = { -2.0f, 0.0f, -6.0f };
		room->walls[1].points[1] = { -5.85f, 0.0f, -6.0f };
		room->walls[1].points[2] = { -6.0f, 0.0f, 6.0f };
		room->walls[1].points[3] = { -2.0f, 0.0f, 6.0f };

		room->cloud = NULL;
		if ( i == 1 ) {
			room->cloud = &cloud;
		}
	}

	MeshGeometryData icosphere = { };
	MakeIcosphere( &icosphere, &gMem->frameAllocater );

	//Player move logic
	const float speedPerSec = 4.0f;
	float moveSpeed = ( speedPerSec / 1000.0f ) * millisecondsElapsed;

	Vec3 moveVec = { };
	if( IsKeyDown( i, 'w' ) ) moveVec.z += 1.0f;
	if( IsKeyDown( i, 's' ) ) moveVec.z -= 1.0f;
	if( IsKeyDown( i, 'a' ) ) moveVec.x += 1.0f;
	if( IsKeyDown( i, 'd' ) ) moveVec.x -= 1.0f;
	if( Vec3Length( moveVec ) > 1e-04 ) { Normalize( &moveVec ); }
	moveVec = moveVec * moveSpeed;
	gMem->playerP = gMem->playerP + moveVec;

	bool pThisFrame = IsKeyDown( i, 'p' );
	bool bubbleWasCast = pThisFrame && !gMem->pLastFrame;
	if( bubbleWasCast && gMem->bubbleRemaining <= 0.0f ) {
		float bubbleTime = 10.0f;
		float bubbleCost = 10.0f;
		gMem->amountOfResource -= bubbleCost;
		gMem->bubbleRemaining += bubbleTime;
	}
	gMem->pLastFrame = pThisFrame;

	if( gMem->bubbleRemaining > 0.0f ) {
		gMem->bubbleRemaining -= millisecondsElapsed / 1000.0f;
	}

	ImGui::Begin( "World State" );

	gMem->colliderRadius = 0.2f;
	gMem->meshScaling = 0.5f;
	//Collision Logic
	{
		Room* currentRoom = &rooms[0][ gMem->roomIndex ];

		for( int i = 0; i < currentRoom->wallCount; ++i ) {
			Room::Wall wall = currentRoom->walls[ i ];
			for( int j = 0; j < wall.pointCount - 1; ++j ) {
				Vec3 p1 = wall.points[ j ];
				Vec3 p2 = wall.points[ j + 1 ];

				Vec3 normalAway = { };
				float dist = DistanceFromLineSeg( gMem->playerP, p1, p2, &normalAway );

				if( dist < gMem->colliderRadius ) {
					float adjustDist = gMem->colliderRadius - dist;
					Vec3 adjustVec = normalAway * adjustDist;

					gMem->playerP = gMem->playerP + adjustVec;
				}
			}
		}
	}

	//Room change Logic
	if( gMem->playerP.z < -6.0f && gMem->roomIndex < 3 ) {
		gMem->roomIndex += 1;
		gMem->playerP.z += roomDimensions.z;
	} else if( gMem->playerP.z > 6.0f && gMem->roomIndex > 0 ) {
		gMem->roomIndex -= 1;
		gMem->playerP.z -= roomDimensions.z;
	}

	//Update Wind state
	if( gMem->roomIndex == 2 ) {
		float windStartUpTime = 1.0f;
		float windChangeThisTic = ( millisecondsElapsed / 1000.0f ) * windStartUpTime;
		gMem->windState += windChangeThisTic;
		windState.intensity = gMem->windState;

		Vec3 windVec = windState.dir * windState.intensity * ( millisecondsElapsed / 1000.0f );
		gMem->playerP = gMem->playerP + windVec;

		ImGui::Text( "Wind Intensity: %.3f", windState.intensity );
	} else {
		gMem->windState = 0.0f;
	}

	//Render wind particles
	{

	}

	//Camera Logic
	float aspectRatio = (float)system->windowWidth / (float)system->windowHeight;
	float viewRange = 3.0f;
	float viewDepth = 13.0f;
	Mat4 projectionMatrix = { };
	CreateOrthoCameraMatrix( 
		viewRange * aspectRatio, 
		viewRange, 
		viewDepth, 
		&projectionMatrix 
	);

	Mat4 viewMatrix = { };
	Vec3 p = { 0.0f, 3.0f, -6.0f };
	Quat q = { };
	{
		Vec3 axis1 = { 0.0f, 0.0f, 1.0f };
		Vec3 axis2 = p;
		axis2.z *= -1.0f;
		Normalize( &axis2 );
		float angle = AngleBetween( axis1, axis2 );

		q = FromAngleAxis( 1.0f, 0.0f, 0.0f, angle );

		Quat correction = FromAngleAxis( 0.0f, 0.0f, 1.0f, PI );
		q = MultQuats( q, correction );
	}
	viewMatrix = CreateViewMatrix( q, p + gMem->playerP );
	Mat4 viewProjectionMat = MultMatrix( viewMatrix, projectionMatrix );

	//Render Player Model
	{
		Mat4 modelMatrix = { };
		SetToIdentity( &modelMatrix );

		modelMatrix[0][0] *= gMem->meshScaling;
		modelMatrix[1][1] *= gMem->meshScaling;
		modelMatrix[2][2] *= gMem->meshScaling;

		float lookRotationAngle = 0.0f; 
		if( Vec3Length( moveVec ) > 1e-04 ) {
			Normalize( &moveVec );

			lookRotationAngle = AngleBetween( moveVec, forwardVec );
			float cross = Cross( moveVec, forwardVec ).y;
			lookRotationAngle *= 1.0f + ( -2.0f * ( cross < 0.0f ) );
		} else {
			lookRotationAngle = gMem->prevFacingAngle;
		}
		gMem->prevFacingAngle = lookRotationAngle;
		Mat4 rotationMat = { };
		Quat lookRotationQuat = FromAngleAxis( 0.0f, 1.0f, 0.0f, lookRotationAngle );
		rotationMat = MatrixFromQuat( lookRotationQuat );

		modelMatrix = MultMatrix( rotationMat, modelMatrix );

		SetTranslation( 
			&modelMatrix, 
			gMem->playerP.x, 
			gMem->playerP.y, 
			gMem->playerP.z 
		);

		int normalInputIndex = GetIndexOfProgramVertexInput( &gMem->shader, "normal" );
		int positionInputIndex = GetIndexOfProgramVertexInput( &gMem->shader, "position" );
		int cameraMatrixIndex = GetIndexOfProgramUniformInput( &gMem->shader, "cameraMatrix" );
		int modelMatrixIndex = GetIndexOfProgramUniformInput( &gMem->shader, "modelMatrix" );

		RenderCommand command = { };
		command.shader = &gMem->shader;
		command.elementCount = gMem->vertexCount;
		command.vertexFormat = RenderCommand::SEPARATE_GPU_BUFFS;
		command.vertexInputData[ normalInputIndex ] = gMem->normalArray;
		command.vertexInputData[ positionInputIndex ] = gMem->vertexArray;
		command.uniformData[ cameraMatrixIndex ] = &viewProjectionMat;
		command.uniformData[ modelMatrixIndex ] = &modelMatrix;

		renderDriver->Draw( &command, false, false );

		if( gMem->bubbleRemaining > 0.0f ) {

			Mat4 bubbleModelMat = { };
			SetToIdentity( &bubbleModelMat );
			SetTranslation( 
				&bubbleModelMat,
				gMem->playerP.x,
				gMem->playerP.y + 0.5f,
				gMem->playerP.z
			);

			float bubbleColor [4] = { 0.77, 0.66, 0.21, 0.5 };
			DrawVertsImmediate( 
				&gMem->primRenderer,
				icosphere.vData,
				icosphere.dataCount,
				false,
				false,
				&viewProjectionMat,
				&bubbleModelMat,
				(float*)bubbleColor
			);
		}
	}

	//Update & Render Room struff
	{
		ImGui::Text( 
			"Player P: %.2f, %.2f, %.2f", 
			gMem->playerP.x, 
			gMem->playerP.y, 
			gMem->playerP.z 
		);

		int roomsRendered = 0;
		for( int roomIndexOffset = -1; roomIndexOffset <= 1; ++roomIndexOffset ) {
			int targetRoomIndex = gMem->roomIndex + roomIndexOffset;
			if( targetRoomIndex < 0 || targetRoomIndex > 3 ) continue;
			Room* room = &rooms[0][ targetRoomIndex ];
			roomsRendered++;

			Vec3 roomOffset = { 
				0.0f, 
				0.0f, 
				-roomDimensions.z * ( targetRoomIndex - gMem->roomIndex ) 
			};
			UpdateAndRenderRoom( room, roomOffset,
				&gMem->primRenderer,
				&viewProjectionMat,
				millisecondsElapsed,
				&gMem->levelOfBadness,
				&gMem->amountOfResource,
				&gMem->badnessPlaying->volume,
				gMem->playerP,
				( gMem->bubbleRemaining > 0.0f )
			);
		}

		ImGui::Text( "Room Index: %d", gMem->roomIndex );
		ImGui::Text( "Rooms Rendered: %d", roomsRendered );
	}

	//World GameState
	ImGui::End();

	//Debug Grid
	DrawGridOnXZPlane( 
		&gMem->primRenderer,
		{ gMem->playerP.x, 0.0f, gMem->playerP.z }, 
		15.0f, 
		&viewProjectionMat
	);

	ImGui::Begin( "Player State" );
	ImGui::Text( "Badness: %.3f", gMem->levelOfBadness );
	ImGui::Text( "Resource: %.3f", gMem->amountOfResource );
	ImGui::Text( "Bubble: %.3f", gMem->bubbleRemaining );
	ImGui::End();

	//TODO: render collision volume on player

	ClearStack( &gMem->frameAllocater );

	ImGui::Render();

    return true;
} 