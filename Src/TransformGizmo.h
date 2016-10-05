struct CameraState {
	Mat4 projection;
	Mat4 viewMatrix;
};

static Mat4 CreatePerspectiveMatrix( 
	float fov, 
	float aspect, 
	float nearPlane,
	float farPlane 
) {
	float depth = farPlane - nearPlane;
	float inverseTanFov = 1.0 / tanf( fov / 2.0 );

	return {
		-inverseTanFov / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, inverseTanFov, 0.0f, 0.0f,
		0.0f, 0.0f, -( ( farPlane + nearPlane ) / depth ), -1.0f,
		0.0f, 0.0f, ( ( farPlane * nearPlane ) / depth ), 1.0f
	};
}

static Mat4 CreateViewMatrix( Quat rotation, Vec3 p ) {
	Vec3 xAxis = ApplyQuatToVec( rotation, { -1.0f, 0.0f, 0.0f } );
	Vec3 yAxis = ApplyQuatToVec( rotation, { 0.0f, -1.0f, 0.0f } );
	Vec3 zAxis = ApplyQuatToVec( rotation, { 0.0f, 0.0f, -1.0f } );

	return {
		xAxis.x, yAxis.x, zAxis.x, 0.0f,
		xAxis.y, yAxis.y, zAxis.y, 0.0f,
		xAxis.z, yAxis.z, zAxis.z, 0.0f,
		-Dot( xAxis, p ), -Dot( yAxis, p ), -Dot( zAxis, p ), 1.0f
	};
}

static inline Mat4 GetViewProjectionMatrix( CameraState* stateInfo ) {
	return MultMatrix( stateInfo->viewMatrix, stateInfo->projection );
}

static Vec3 GetRayFromScreenCoords( 
	Vec2 screenCoords, 
	Mat4 projectionMatrix, 
	Mat4 viewMatrix 
) {
	Mat4 inverseProjection = InverseMatrix( projectionMatrix );
	Mat4 inverseView = InverseMatrix( viewMatrix );

	Vec3 p = { screenCoords.x, screenCoords.y, 0.66f };
	p = MultVec( inverseProjection, p );
	p = MultVec( inverseView, p, 0.0f );
	Normalize( &p );
	return p;
}

static int RayPlaneIntersection( Vec3 plane_p, Vec3 plane_n, Vec3 ray_p, Vec3 ray_n, Vec3* intersectionPoint ) {
	if( Dot( ray_n, plane_n ) > 0.0f ) { //We want to detect intersection even if the plane is facing "away"
		plane_n = plane_n * 1.0f;
	}

	Vec3 rayToPlane = plane_p - ray_p;
	float denom = Dot( ray_n, plane_n );
	if( denom != 0.0f ) {
		float d = Dot( rayToPlane, plane_n ) / denom;
		if( d > 0.0f ) {
			*intersectionPoint = ray_p + ( ray_n * d );
			return 1;
		}
	}
	return 0;
}

struct GizmoInput {
	bool inputButtonState;
	bool toggleStateInput;
	Vec2 mousePosition;
	Vec2 mouseMove;
	Vec3 cameraPos;
	Mat4 projectionMat;
	Mat4 viewMatrix;
};

enum GizmoMode {
	Translate, Rotate
};

static struct {
	int mode;
	bool hasValidInfo;
	bool inputButtonLastUpdate;
	int activeTransformIndex;
	int highlightTransformIndex;
	float radius;

	struct {
		Vec3 clickIntersection;
		Vec3 intersectionThisUpdate;
		Quat retainedStartingRotation;
	} Rotation;

	struct {
		float savedUnitLength;
		Vec2 clickPoint;
		Vec3 savedStartPosition;
	} Translate;

} GizmoState = {
	GizmoMode::Translate,
	false,
	false,
	-1,
	-1,
	0.3f,
	{
		
	},
	{ 

	}
};

void UpdateRotationGizmo( Vec3 position, Quat* rotation, GizmoInput gInput ) {
	Vec3 rotationAxes [3] = {
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f }
	};
	struct IntersectCheck {
		Vec3 intersection;
		int rayPlaneReturn;
	};
	IntersectCheck checks [3] = { };
	int select = -1;

	Vec3 mouseRay = GetRayFromScreenCoords( gInput.mousePosition, gInput.projectionMat, gInput.viewMatrix );

	float compare = -1.0f;
	for( int i = 0; i < 3; ++i ) {
		if( GizmoState.hasValidInfo ) {
			rotationAxes[i] = ApplyQuatToVec( GizmoState.Rotation.retainedStartingRotation, rotationAxes[i] );
			Normalize( &rotationAxes[i] );
		} else {
			rotationAxes[i] = ApplyQuatToVec( *rotation, rotationAxes[i] );
			Normalize( &rotationAxes[i] );
		}

		checks[i].rayPlaneReturn = RayPlaneIntersection( position, rotationAxes[i], gInput.cameraPos, mouseRay, &checks[i].intersection );

		if( checks[i].rayPlaneReturn == 1 ) {
			float dist = Vec3Length( position - checks[i].intersection );
			if( dist <= ( GizmoState.radius * 1.1 ) && !GizmoState.hasValidInfo ) {
				float distFromBorder = GizmoState.radius - dist;
				distFromBorder = sqrtf( distFromBorder * distFromBorder );
				if( compare < 0.0f || distFromBorder < compare ) {
					select = i;
					compare = distFromBorder;
				}
			}
		}
	}

	//Managing of retained state
	bool doRotateWasClicked = !GizmoState.inputButtonLastUpdate && gInput.inputButtonState;
	bool doRotateWasReleased = GizmoState.inputButtonLastUpdate && !gInput.inputButtonState;
	GizmoState.inputButtonLastUpdate = gInput.inputButtonState;

	if( doRotateWasClicked && select != -1 ) {
		GizmoState.activeTransformIndex = select;
		GizmoState.Rotation.clickIntersection = checks[ GizmoState.activeTransformIndex ].intersection;
		GizmoState.Rotation.retainedStartingRotation = *rotation;
		GizmoState.hasValidInfo = true;
	}

	GizmoState.highlightTransformIndex = select;
	//If we've started rotating, and have enough info to do a rotation
	if( GizmoState.hasValidInfo && checks[ GizmoState.activeTransformIndex ].rayPlaneReturn ) {

		float angle = 0.0f;

		//Marked for bad-raycast-refactor
		GizmoState.highlightTransformIndex = GizmoState.activeTransformIndex;
		GizmoState.Rotation.intersectionThisUpdate = checks[ GizmoState.activeTransformIndex ].intersection;
		Vec3 vecToClickIntersect = GizmoState.Rotation.clickIntersection - position;
		Vec3 vecToThisUpdateIntersect = GizmoState.Rotation.intersectionThisUpdate - position;
		Normalize( &vecToClickIntersect );
		Normalize( &vecToThisUpdateIntersect );

		angle = AngleBetween( vecToClickIntersect, vecToThisUpdateIntersect );
		Vec3 crossed = Cross( vecToClickIntersect, vecToThisUpdateIntersect );
		Normalize( &crossed );

		Vec3 rotationAxis = rotationAxes[ GizmoState.activeTransformIndex ];
		float sign = Dot( rotationAxis, crossed );
		if( sign < 0.0f ) { 
			angle *= -1.0f;
		}
		//End Mark

		Quat rotationToApply = FromAngleAxis( rotationAxis, angle );
		*rotation = MultQuats( rotationToApply, GizmoState.Rotation.retainedStartingRotation );
	}

	if( doRotateWasReleased ) {
		GizmoState.Rotation.clickIntersection = { };
		GizmoState.Rotation.intersectionThisUpdate = { };
		GizmoState.Rotation.retainedStartingRotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		GizmoState.activeTransformIndex = -1;
		GizmoState.hasValidInfo = false;
	}
}

void UpdateTranslationGizmo( Vec3* position, GizmoInput gInput ) {
	const Vec3 translationAxes [3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
	};
	Vec2 onScreenAxes [3] = {
	};

	//this assumes the following projection mat: 
	/*{
		-inverseTanFov / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, inverseTanFov, 0.0f, 0.0f,
		0.0f, 0.0f, -( ( farPlane + nearPlane ) / depth ), -1.0f,
		0.0f, 0.0f, ( ( 2.0f * farPlane * nearPlane ) / depth ), 1.0f
	}; */
	gInput.projectionMat[0][0] *= -1.0f;
	gInput.projectionMat[1][1] *= -1.0f;
	gInput.projectionMat[2][2] *= -1.0f;
	gInput.projectionMat[3][2] = -1.0f;
	gInput.projectionMat[3][3] = 0.0f;


	Mat4 viewProjectionMatrix = MultMatrix(  gInput.viewMatrix, gInput.projectionMat );
	Vec3 p = *position;
	p = MultVec( viewProjectionMatrix, p );
	Vec2 position2D = { p.x / p.z, p.y / p.z };

	Vec2 dirToMouseFromP = gInput.mousePosition - position2D;
	float distanceFromP = Vec2Len( dirToMouseFromP );
	Normalize( &dirToMouseFromP );

	int select = -1;
	float dotCompare = 0.0f;
	float distanceCutoff = 0.5f;
	float dotCutoff = 0.5f;

	if( distanceFromP > distanceCutoff && !GizmoState.hasValidInfo ) goto skiploop;
	for( int i = 0; i < 3; ++i ) {
		Vec3 axis = translationAxes[i];
		Vec3 ep = MultVec( viewProjectionMatrix, ( *position + axis ) );
		Vec2 endPoint2D = { ep.x / ep.z, ep.y / ep.z };
		Vec2 onScreenAxis = endPoint2D - position2D;
		float axisLength = Vec2Len( onScreenAxis ) * 0.5f;
		Normalize( &onScreenAxis );
		onScreenAxes[i] = onScreenAxis;

		float dotToAxis = abs( Dot( onScreenAxis, dirToMouseFromP ) );
		if( dotToAxis > dotCompare && dotToAxis > dotCutoff ) {
			dotCompare = dotToAxis;
			select = i;
		}
	}
	skiploop:

	GizmoState.highlightTransformIndex = select;

	static bool mouseButtonLastState = false;
	bool mouseWasPressed = !mouseButtonLastState && gInput.inputButtonState;
	bool mouseWasReleased = mouseButtonLastState && !gInput.inputButtonState;
	mouseButtonLastState = gInput.inputButtonState;

	if( !GizmoState.hasValidInfo && select != -1 && mouseWasPressed ) {
		GizmoState.activeTransformIndex = select;
		GizmoState.Translate.clickPoint = gInput.mousePosition;
		GizmoState.Translate.savedStartPosition = *position;
		GizmoState.Translate.savedUnitLength = Vec2Len( position2D - onScreenAxes[ GizmoState.activeTransformIndex ] );
		GizmoState.hasValidInfo = true;
	} else if( mouseWasReleased ) {
		GizmoState.activeTransformIndex = -1;
		GizmoState.Translate.clickPoint = { };
		GizmoState.Translate.savedStartPosition = { };
		GizmoState.Translate.savedUnitLength = 0.0f;
		GizmoState.hasValidInfo = false;
	}

	if( GizmoState.hasValidInfo ) {
		Vec2 dragged = gInput.mousePosition - GizmoState.Translate.clickPoint;
		float dragLength = Vec2Len( dragged );
		float move = dragLength / GizmoState.Translate.savedUnitLength;

		if( Dot( dragged, onScreenAxes[ GizmoState.activeTransformIndex ] ) < 0.0f ) {
			move *= -1.0f;
		}

		*position = GizmoState.Translate.savedStartPosition + ( translationAxes[ GizmoState.activeTransformIndex ] * move );

		GizmoState.highlightTransformIndex = GizmoState.activeTransformIndex;
	}
}

#if 0
void RenderRotationGizmo( Vec3 position, Quat rotation, Mat4 cameraMatrix, DebugDraw* debugDraw ) {
	const float radius = GizmoState.radius;

	Vec3 xyCircle [13];
	for( int i = 0; i < 13; ++i ) {
		float rads = ( ( (float)i ) / 12.0f ) * PI * 2.0f;
		xyCircle[i] = { cosf( rads ) * radius, sinf( rads ) * radius, 0.0f };
		xyCircle[i] = ApplyQuatToVec( rotation, xyCircle[i] );
		xyCircle[i] = xyCircle[i] + position;
	}

	Vec3 xzCircle [13];
	for( int i = 0; i < 13; ++i ) {
		float rads = ( ( (float)i ) / 12.0f ) * PI * 2.0f;
		xzCircle[i] = { cosf( rads ) * radius, 0.0f, sinf( rads ) * radius };
		xzCircle[i] = ApplyQuatToVec( rotation, xzCircle[i] );
		xzCircle[i] = xzCircle[i] + position;
	}

	Vec3 yzCircle [13];
	for( int i = 0; i < 13; ++i ) {
		float rads = ( ( (float)i ) / 12.0f ) * PI * 2.0f;
		yzCircle[i] = { 0.0f, cosf( rads ) * radius, sinf( rads ) * radius };
		yzCircle[i] = ApplyQuatToVec( rotation, yzCircle[i] );
		yzCircle[i] = yzCircle[i] + position;
	}

	Vec3 colors [6] = {
		{ 0.69f, 0.12f, 0.12f },
		{ 0.89f, 0.32f, 0.32f },
		{ 0.13f, 0.67f, 0.12f },
		{ 0.5f, 0.89f, 0.48f },
		{ 0.12f, 0.12f, 0.67f },
		{ 0.45f, 0.47f, 0.91f }
	};

	int colorselect = 0 + ( GizmoState.highlightTransformIndex == 0 );
	RenderDebugLines( 
		debugDraw, 
		(float*)xyCircle, 
		13, 
		&cameraMatrix, 
		colors[ colorselect ] 
	);

	colorselect = 2 + ( GizmoState.highlightTransformIndex == 1 );
	RenderDebugLines( 
		debugDraw, 
		(float*)xzCircle, 
		13, 
		&cameraMatrix, 
		colors[ colorselect ] 
	);

	colorselect = 4 + ( GizmoState.highlightTransformIndex == 2 );
	RenderDebugLines( 
		debugDraw, 
		(float*)yzCircle, 
		13, 
		&cameraMatrix, 
		colors[ colorselect ] 
	);

	if( GizmoState.hasValidInfo ) {
		Vec3 rotationLines [4] = {
			GizmoState.Rotation.clickIntersection,
			position,
			position,
			GizmoState.Rotation.intersectionThisUpdate
		};
		RenderDebugLines( 
			debugDraw, 
			(float*)rotationLines, 
			4, 
			&cameraMatrix, 
			{ 0.08, 0.89, 0.4 } 
		);
	}
}

void RenderTranslateGizmo( Vec3 position, Mat4 viewProjectionMatrix, DebugDraw* debugDraw ) {
	Vec3 x = { 1.0f, 0.0f, 0.0f };
	Vec3 y = { 0.0f, 1.0f, 0.0f };
	Vec3 z = { 0.0f, 0.0f, 1.0f };
	const float lineLengths = 1.0f;
	Vec3 lines [6] = {
		position + ( x * 0.5f ),
		position + ( x * -0.5f ),
		position + ( y * 0.5f ),
		position + ( y * -0.5f ),
		position + ( z * 0.5f ),
		position + ( z * -0.5f ),
	};
	Vec3 colors [6] = {
		{ 0.69, 0.12, 0.12 },
		{ 0.89, 0.32, 0.32 },
		{ 0.13, 0.67, 0.12 },
		{ 0.5, 0.89, 0.48 },
		{ 0.12, 0.12, 0.67 },
		{ 0.45, 0.47, 0.91 }
	};

	int cIndex = 0;
	cIndex += 1 * ( GizmoState.highlightTransformIndex == 0 );
	RenderDebugLines( 
		debugDraw, 
		(float*)&lines[0], 
		2, 
		&viewProjectionMatrix, 
		colors[ cIndex ] 
	);

	cIndex = 0;
	cIndex += 1 * ( GizmoState.highlightTransformIndex == 1 );
	RenderDebugLines( 
		debugDraw, 
		(float*)&lines[2], 
		2, 
		&viewProjectionMatrix, 
		colors[ cIndex ] 
	);

	cIndex = 0;
	cIndex += 1 * ( GizmoState.highlightTransformIndex == 2 );
	RenderDebugLines( 
		debugDraw, 
		(float*)&lines[4], 
		2, 
		&viewProjectionMatrix, 
		colors[ cIndex ] 
	);
}
#endif

void UpdateGizmo( Vec3* position, Quat* rotation, GizmoInput gInput ) {
	if( gInput.toggleStateInput ) {
		switch( GizmoState.mode ) {
			case GizmoMode::Translate : 
			GizmoState.mode = GizmoMode::Rotate;
			break;

			case GizmoMode::Rotate :
			GizmoState.mode = GizmoMode::Translate;
			break;
		}
	}

	switch ( GizmoState.mode ) {
		case GizmoMode::Translate:
		UpdateTranslationGizmo( position, gInput );
		break;

		case GizmoMode::Rotate:
		UpdateRotationGizmo( *position, rotation, gInput );
		break;
	}
}

void RenderGizmo( Vec3 position, Quat rotation, Mat4 viewProjectionMatrix, DebugDraw* debugDraw ) {
	#if 0
	switch ( GizmoState.mode ) {
		case GizmoMode::Translate:
		RenderTranslateGizmo( position, viewProjectionMatrix, debugDraw );
		break;

		case GizmoMode::Rotate:
		RenderRotationGizmo( position, rotation, viewProjectionMatrix, debugDraw );
		break;
	}
	#endif
}