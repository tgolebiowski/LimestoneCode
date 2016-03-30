struct Vec2 {
	float x, y;
};

float Len( const Vec2 v ) {
	return sqrtf( v.x * v.x + v.y * v.y );
};

void Normalize( Vec2* v ) {
	float lenInv = 1.0f / Len( *v );
	v->x *= lenInv;
	v->y *= lenInv;
};

float Dot( const Vec2 a, const Vec2 b ) {
	return a.x * b.x + a.y * b.y;
};

///Vec a & b must be normalized already
///Implementation credit to Jon Blow
Vec2 Slerp2D( const Vec2 vecA, const Vec2 vecB, const float t ) {
	float dot = Dot( vecA, vecB );
	float theta = acosf(dot) * t; //halfway between the two vecs
	Vec2 relativeVec = { vecB.x - vecA.x * dot, vecB.y - vecA.y * dot };
	Normalize( &relativeVec );
	float thetaSin = sinf( theta ); float thetaCos = cosf( theta );
	return { vecA.x * thetaCos + relativeVec.x * thetaSin, vecA.y * thetaCos + relativeVec.y * thetaSin };
};

struct AI_Component {
	Vec2 pos;
	Vec2 moveDir;
	Vec2 avoidDir;
};

struct AIDataStorage {
	AI_Component aiEntities[2];
	uint8 entityCount;
};

void InitAIComponents( AIDataStorage* storage ) {
	storage->aiEntities[0].pos = { 1.0f, 3.0f };
	storage->aiEntities[1].pos = { -1.0f, 3.0f };
	storage->entityCount = 2;
}

void UpdateAI( AIDataStorage* storage, Vec3 targetPos ) {
	for( uint8 entityIndex = 0; entityIndex < storage->entityCount; entityIndex++ ) {
		AI_Component* e = &storage->aiEntities[ entityIndex ];
		Vec2 vecToTarget = { targetPos.x - e->pos.x, targetPos.y - e->pos.y };
		Vec2 normalizedVecTo = vecToTarget; Normalize( &normalizedVecTo );
		const float factor = 2.0f;
		float distanceFromTarget = Len( vecToTarget );
		float ellipseDistance = distanceFromTarget * factor;
		float falloffEllipseDistance = distanceFromTarget * falloffEllipseDistance;

		e->moveDir = normalizedVecTo;
		e->avoidDir = normalizedVecTo;

		for( uint8 otherIndex = 0; otherIndex < storage->entityCount; otherIndex++ ) {
			if( otherIndex == entityIndex ) continue;
			AI_Component* o = &storage->aiEntities[ otherIndex ];

			Vec2 vecToTarget_o = { targetPos.x - o->pos.x, targetPos.y - o->pos.y };
			Vec2 awayFromO = { e->pos.x - o->pos.x, e->pos.y - o->pos.y };
			float distanceFromTarget_o = Len( vecToTarget_o );
			float distBetween = Len( awayFromO );

			float ellipseDistance_o = distanceFromTarget_o + distBetween;
			if( ellipseDistance_o <= ellipseDistance ) {
	            //TODO: fix these names, they're awful
	            float viableDistanceRange = ellipseDistance - distanceFromTarget; 
	            float comparedDistanceValue = ellipseDistance_o - distanceFromTarget;

	            float normalizedE_Dist = comparedDistanceValue / viableDistanceRange;
	            float ellipseFalloffStart = ellipseDistance * 0.9;
	            float edgeFalloffFactor = 1.0f;
	            if( ellipseDistance_o > ellipseFalloffStart ) {
	            	edgeFalloffFactor *= ( 1.0f - ( ellipseDistance_o - ellipseFalloffStart ) / ( ellipseDistance - ellipseFalloffStart ) );
	            	sqrtf( edgeFalloffFactor );
            	}
	            
            	Vec2 normalizedAwayVec = awayFromO; Normalize( &normalizedAwayVec );
            	e->avoidDir = Slerp2D( e->avoidDir, normalizedAwayVec, normalizedE_Dist * edgeFalloffFactor );
	        }
		}

		float moveSpeed = 0.01f;
		e->pos.x += e->avoidDir.x * moveSpeed;
		e->pos.y += e->avoidDir.y * moveSpeed;
	}
}

void DebugRenderAI( AIDataStorage* storage ) {
	Vec3 avoidData[16];
	Vec3 toVecData[16];
	uint8 avoidLineDataCount = 0;
	uint8 toVecLineDataCount = 0;
	for( uint8 entityIndex = 0; entityIndex < storage->entityCount; entityIndex++ ) {
		AI_Component* e = &storage->aiEntities[ entityIndex ];
		Vec2 p1 = storage->aiEntities[entityIndex].pos;
		RenderDebugCircle( { p1.x, p1.y, 0.0f }, 0.1f );

		toVecData[ toVecLineDataCount++ ] = { p1.x, p1.y, 0.0f };
		toVecData[ toVecLineDataCount++ ] = { p1.x + e->moveDir.x * 0.5f, p1.y + e->moveDir.y * 0.5f, 0.0f };
		avoidData[ avoidLineDataCount++ ] = { p1.x, p1.y, 0.0f };
		avoidData[ avoidLineDataCount++ ] = { p1.x + e->avoidDir.x * 0.5f, p1.y + e->avoidDir.y * 0.5f, 0.0f };
	}

	Mat4 i; SetToIdentity( &i );
	RenderDebugLines( (float*)&toVecData[0], toVecLineDataCount, i, { 0.06, 0.89, 0.06 } );
	RenderDebugLines( (float*)&avoidData[0], avoidLineDataCount, i, { 0.89, 0.06, 0.06 } );
}