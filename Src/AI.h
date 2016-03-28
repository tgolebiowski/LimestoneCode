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

		e->moveDir = normalizedVecTo;
		e->avoidDir = normalizedVecTo;

		for( uint8 otherIndex = 0; otherIndex < storage->entityCount; otherIndex++ ) {
			if( otherIndex == entityIndex ) continue;
			AI_Component* o = &storage->aiEntities[ otherIndex ];

			Vec2 vecToTarget_o = { targetPos.x - o->pos.x, targetPos.y - o->pos.y };
			float targetVecCompareDot = Dot( vecToTarget, vecToTarget_o );
			if( targetVecCompareDot <= 0.0f ) continue;

			Vec2 awayFromO = { e->pos.x - o->pos.x, e->pos.y - o->pos.y };
			float distanceFromTarget_o = Len( vecToTarget_o );
			float distBetween = Len( awayFromO );

			float ellipseDistance_o = distanceFromTarget_o + distBetween;
			if( ellipseDistance_o <= ellipseDistance ) {
	            Vec2 normalizedVecTo_O = vecToTarget_o; Normalize( &normalizedVecTo_O );
	            Vec2 normalizedAwayVec = awayFromO; Normalize( &normalizedAwayVec );
	            float normalizedDot = 1.0f - Dot( normalizedVecTo, normalizedVecTo_O );
	            float normalizedE_Dist = ( ellipseDistance_o - distanceFromTarget ) / ( ellipseDistance - distanceFromTarget );

	            e->avoidDir = Slerp2D( e->avoidDir, normalizedAwayVec, normalizedDot * normalizedE_Dist );
	        }
		}
	}
}

void DebugRenderAI( AIDataStorage* storage ) {
	Vec3 lineData[64];
	uint8 lineDataCount = 0;
	for( uint8 entityIndex = 0; entityIndex < storage->entityCount; entityIndex++ ) {
		AI_Component* e = &storage->aiEntities[ entityIndex ];
		Vec2 p1 = storage->aiEntities[entityIndex].pos;
		RenderDebugCircle( { p1.x, p1.y, 0.0f }, 0.1f );

		lineData[ lineDataCount++ ] = { p1.x, p1.y, 0.0f };
		lineData[ lineDataCount++ ] = { p1.x + e->moveDir.x * 0.5f, p1.y + e->moveDir.y * 0.5f, 0.0f };
		lineData[ lineDataCount++ ] = { p1.x, p1.y, 0.0f };
		lineData[ lineDataCount++ ] = { p1.x + e->avoidDir.x * 0.5f, p1.y + e->avoidDir.y * 0.5f, 0.0f };
	}

	Mat4 i; SetToIdentity( &i );
	RenderDebugLines( (float*)&lineData[0], lineDataCount, i );
}