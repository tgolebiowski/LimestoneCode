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
};

void InitAIComponents( AIDataStorage* storage ) {
	storage->aiEntities[0].pos = { -0.0f, 0.0f };
	storage->aiEntities[1].pos = { 0.5f, 0.5f };
}

void UpdateAI( AIDataStorage* storage, Vec3 targetPos ) {

}

void DebugRenderAI( AIDataStorage* storage ) {
	Vec2 p1 = storage->aiEntities[0].pos;
	RenderDebugCircle( { p1.x, p1.y, 0.0f }, 0.025 );
}