struct Point {
	float x,y;
};

struct FakeEnemy {
    float x, y;
    float pathData[3 * 32];
    uint32 pathIndex[32];
    uint16 pathCount;
};

struct MagFieldMem {
	FakeEnemy f[4];
    Point targetPoint;
    Point negPoints[4];
    uint8 negPointCount;
};

void InitFieldMem( MagFieldMem* gMem ) {
	gMem->f[0].x = -120.0f;
    gMem->f[0].y = -100.0f;
    gMem->f[1].x = -130.0f;
    gMem->f[1].y = 100.0f;
    gMem->f[2].x = -120.0f;
    gMem->f[2].y = -140.0f;
    gMem->f[3].x = -140.0f;
    gMem->f[3].y = 180.0f;
    gMem->targetPoint = { 0.0f, 0.0f };
    gMem->negPointCount = 4;
}

Point GetFieldDir( const Point* antiPoints, uint8 antiCount, const Point targetPoint, const Point p ) {
	Point returnVec;

	float xDiff = targetPoint.x - p.x;
	float yDiff = targetPoint.y - p.y;
	float r_sqrdInv_target = 1.0f / (xDiff * xDiff + yDiff * yDiff);
	returnVec = { xDiff * r_sqrdInv_target, yDiff * r_sqrdInv_target };

	for( uint8 antiP_Index = 0; antiP_Index < antiCount; antiP_Index++ ) {
		Point arbNegPoint = antiPoints[antiP_Index];
		if(arbNegPoint.x == p.x && arbNegPoint.y == p.y ) continue;

		xDiff = p.x - arbNegPoint.x;
		yDiff = p.y - arbNegPoint.y;
		float sqrdLen = (xDiff * xDiff + yDiff * yDiff);
		const float maxRepelLen = 50.0f;
		if( sqrdLen < maxRepelLen * maxRepelLen ) {
			float r_sqrdInv_neg = 1.0f / sqrdLen;
			returnVec.x += xDiff * r_sqrdInv_neg;
			returnVec.y += yDiff * r_sqrdInv_neg;
		}
	}

	float invSqr = 1.0f / sqrtf( returnVec.x * returnVec.x + returnVec.y * returnVec.y );
	returnVec.x *= invSqr;
	returnVec.y *= invSqr;

	return returnVec;
}

void UpdateMem( MagFieldMem* gMem ) {

    float x, y;
    GetMousePosition( &x, &y );
    if( x < -1.0f || x > 1.0f ) x = 0.0f;
    if( y < -1.0f || y > 1.0f ) y = 0.0f;

    x *= (640 / 2);
    y *= (480 / 2);

    gMem->targetPoint.x = x;
    gMem->targetPoint.y = y;

    for( uint8 i = 0; i < 4; i++ ) {
        gMem->negPoints[i] = { gMem->f[i].x, gMem->f[i].y };
    }

    for( int i = 0; i < 4; i++ ) {
        FakeEnemy* f = &gMem->f[i];

        f->pathCount = 0;
        float current_x = f->x;
        float current_y = f->y;
        bool endFlag = true;

        const float moveSpeed = 2.0f;
        Point moveDir = GetFieldDir( (Point*)&gMem->negPoints, gMem->negPointCount, gMem->targetPoint, { f->x, f->y } );
        f->x += moveDir.x;
        f->y += moveDir.y;

        while(endFlag && current_x >= -640 / 2 && current_x <= 640 / 2 && current_y >= - 480 / 2 && current_y <= 480 / 2) {
            f->pathData[f->pathCount * 3] = current_x;
            f->pathData[f->pathCount * 3 + 1] = current_y;
            f->pathData[f->pathCount * 3 + 2] = 0.0f;
            f->pathIndex[f->pathCount] = f->pathCount;
            f->pathCount++;

            moveDir = GetFieldDir( (Point*)&gMem->negPoints, gMem->negPointCount, gMem->targetPoint, { current_x, current_y } );
            current_x += moveDir.x * moveSpeed;
            current_y += moveDir.y * moveSpeed;

            if(f->pathCount == 1) {
                f->x = current_x;
                f->y = current_y;
            }

            endFlag = !(std::abs( current_x - gMem->targetPoint.x ) < 1e-06 && std::abs( current_y - gMem->targetPoint.y ) < 1e-06) && f->pathCount < 32;
        }
    }
}

void RenderMagStuff( MagFieldMem* gMem ) {
	RenderCircle( { gMem->targetPoint.x, gMem->targetPoint.y, 0.0f }, 8.0f );
    RenderCircle( { gMem->negPoints[0].x, gMem->negPoints[0].y, 0.0f }, 6.0f );

    for( int i = 0; i < 4; i++ ) {
        FakeEnemy* f = &gMem->f[i];
        RenderCircle( { f->x, f->y, 0.0f }, 12.0f );
        RenderLineStrip( (float*)&f->pathData, (uint32*)&f->pathIndex, f->pathCount );
    }
}