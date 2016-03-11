
struct Site {
	float x,y;
};

float Distance2D( Site s1, Site s2 ) {
	float xDiff = s2.x - s1.x;
	float yDiff = s2.y - s1.y;
	return sqrtf( xDiff * xDiff + yDiff * yDiff );
}

struct Edge {
	Site* s1;
	Site* s2;
	};

struct Entity {
	Site p;
	Site pathSites[16];
	Edge pathEdges[16];
	uint8 pathCount;
};

struct AngleOrderNode {
	float angle;
	AngleOrderNode* next;
	AngleOrderNode* prev;
	void* data;
};

struct DistanceEvent {
	float distance;
	DistanceEvent* next;
	DistanceEvent* prev;
	AngleOrderNode* a_ptr;
};

void CreateSweepPaths( Entity* entities, uint8 entityCount, Site target ) {
	const uint8 MAX_NODES = 16;
	DistanceEvent d_nodes[MAX_NODES];
	AngleOrderNode a_nodes[MAX_NODES];
	DistanceEvent* d_head = NULL;
	DistanceEvent* d_tail = NULL;
	AngleOrderNode* a_head = NULL;
	AngleOrderNode* a_tail = NULL;
	uint8 nodeCount = 0;

	//Initializing events and nodes
	for( int i = 0; i < entityCount; i++ ) {
		entities[i].pathSites[0] = target;
		entities[i].pathSites[1] = entities[i].p;
		entities[i].pathEdges[0].s1 = &orderedList->pathSite[0];
		entities[i].pathEdges[0].s2 = &orderedList->pathSite[1];
		entities[i].pathCount = 1;
		void* e_add = (void*)&entities[i];

		DistanceEvent* thisDist = &d_nodes[nodeCount];
		AngleOrderNode* thisAngle = &a_nodes[nodeCount];
		thisDist->next = NULL;
		thisDist->prev = NULL;
		thisAngle->next = NULL;
		thisAngle->prev = NULL;
		thisDist->data = e_add;
		thisDist->data = thisAngle;
		nodeCount++;
		assert(nodeCount < MAX_NODES);

		float xDiff = target.x - entities[i].p.x;
		float yDiff = target.y - entities[i].p.y;
		thisDist->distance = sqrtf( xDiff * xDiff + yDiff * yDiff ); 

		float invDist = 1.0f / thisDist->distance;
		xDiff *= invDist;
		yDiff *= invDist;
		assert(thisDist->distance > 1e-06);
		thisAngle->angle = atan2f( yDiff, xDiff );

		//TODO: BST search rather than linear search
		if( d_head == NULL && a_head == NULL ) {
			d_head = thisDist;
			a_head = thisAngle;
		} else {
			DistanceEvent* current = d_head;
			DistanceEvent* last = NULL;
			for(;;) {
				if( current == NULL ){
					thisDist->prev = last;
					last->next = thisDist;
					d_tail = thisDist;
					break;
				} else if( current->distance > thisDist->distance ) {
					thisDist->next = current;
					thisDist->prev = current->prev;
					current->prev = thisDist;
					if(last != NULL) {
						last->next = thisDist;
					} else {
						d_head = thisDist;
					}
					break;
				}
				last = current;
				current = current->next;
			}

			AngleOrderNode* a_current = a_head;
			AngleOrderNode* a_last = NULL;
			for(;;) {
				if( a_current == NULL) {
					thisAngle->prev = a_last;
					a_last->next = thisAngle;
					a_tail = thisAngle;
					break;
				} else if ( a_current->angle > thisAngle->angle ) {
					a_current->prev = thisAngle;
					thisAngle->next = a_current;
					thisAngle->prev = a_last;
					if( a_last == NULL) {
						a_head = thisAngle;
					}else {
						a_last->next = thisAngle;
					}
					break;
				}
				a_last = a_current;
				a_current = a_current->next;
			}
		}
	}

	// float lastDistance = 0.0f;
	// DistanceEvent* current_d = d_head;
	// while( current_d != NULL ) {
	// 	float distanceDiff = current_d->distance - lastDistance;

	// 	lastDistance = current_d;
	// 	current_d = current_d->next;
	// };

	DistanceEvent* current_d = d_tail;
	while( current_d != NULL ) {
		float distanceDelta = current_d->distance - current_d->prev->distance;

		current_d = current_d->prev;
	}
}