#include "micropather.h"
#include "micropather.cpp"

#define MAP_HORZ_SEGS 12
#define MAP_VERT_SEGS 9

struct Vec2 {
	float x, y;
};

bool LineSegIntersection( Vec2 a, Vec2 b, Vec2 c, Vec2 d ) {
	struct N {
		static bool onSegment( Vec2 a, Vec2 b, Vec2 c ) {
			return (a.x <= b.x || a.x <= c.x) && ( a.x >= b.x || a.x >= c.x ) &&
			       (a.y <= b.y || a.y <= c.y) && ( a.y >= b.y || a.y >= b.y );
		};

        // To find orientation of ordered triplet (p, q, r).
        // 0 --> p, q and r are colinear
        // 1 --> Clockwise
        // 2 --> Counterclockwise
		static int Orientation( Vec2 a, Vec2 b, Vec2 c ) {
			int val = (b.y - a.y) * (c.x - b.x) - 
			(b.x - a.x) * (c.y - b.y);

			if( val == 0 ) return 0;
			else if( val > 0 ) return 1;
			else return 2;
		};
	};

	uint8 o1 = N::Orientation( a, c, b );
	uint8 o2 = N::Orientation( a, d, b );
	uint8 o3 = N::Orientation( c, a, d );
	uint8 o4 = N::Orientation( c, b, d );

	//General Case
	if( o1 != o2 && o3 != o4 ) return true;

	//c lies on segment a-b colinearly
	if( o1 == 0 && N::onSegment(c, a, b)) return true;
	//d lies on segment a-b colinearly
	if( o2 == 0 && N::onSegment(d, a, b)) return true;
	//a lies on segment c-d colinearly
	if( o3 == 0 && N::onSegment(a, c, d)) return true;
	//b lies on segment c-d colinearly
	if( o4 == 0 && N::onSegment(b, c, d)) return true;

	return false;
}

struct MapNode {
	Vec2 location;
	uint16 xIndex, yIndex;
	float influence;
	MapNode* neighbors [8];
	uint8 neighborCount;
};

struct AI_Entity {
	Vec2 p;
	Vec2 currentPath [32];
	uint8 pathCount;
};

struct AIMap : public micropather::Graph {
	float LeastCostEstimate( void* startState, void* endState );
	void AdjacentCost( void* state, MP_VECTOR< micropather::StateCost >* adjacent );
	void PrintStateInfo( void* state ) {};

	MapNode allNodes[ MAP_VERT_SEGS ][ MAP_HORZ_SEGS ];
	float mapXMin, mapXMax, mapYMin, mapYMax, mapXStep, mapYStep;
	AI_Entity* entities;
	uint16 xBoundHigh, xBoundLow, yBoundHigh, yBoundLow;
};

struct AITestMem {
	AI_Entity entities [4];
};

static AIMap aiMap;

float AIMap::LeastCostEstimate( void* startState, void* endState ) {
	MapNode* startNode = (MapNode*)startState;
	MapNode* endNode = (MapNode*)endState;

	float xDiff = endNode->location.x - startNode->location.x;
	float yDiff = endNode->location.y - startNode->location.y;

	return sqrtf( xDiff * xDiff + yDiff * yDiff );
}

void AIMap::AdjacentCost( void* state, MP_VECTOR< micropather::StateCost >* adjacent ){
	MapNode* node = (MapNode*)state;
	Vec2 p1 = node->location;

	for( int n_index = 0; n_index < node->neighborCount; n_index++ ) {
		MapNode* n_node = node->neighbors[n_index];

		assert(n_node >= (MapNode*)&aiMap.allNodes && n_node <= (MapNode*)&aiMap.allNodes[MAP_VERT_SEGS][MAP_HORZ_SEGS]);
		Vec2 p2 = n_node->location;

		float intersectionCost = 0.0f;
		for( int entityIndex = 0; entityIndex < 4; entityIndex++ ) {
			AI_Entity* e = &entities[entityIndex];

			for( int pathIndex = 0; pathIndex < e->pathCount - 2; pathIndex++ ){
				Vec2 p3 = e->currentPath[pathIndex];
				Vec2 p4 = e->currentPath[pathIndex + 1];

				if(p3.x == p2.x && p3.y == p2.y) {
					intersectionCost += 100.0f;
				}

				if(LineSegIntersection(p1, p2, p3, p4)) {
					intersectionCost += 100.0f;
					break;
				}
			}
		}

		if(intersectionCost == 0.0f) {
			micropather::StateCost cost;
			cost.state = (void*)n_node;
			cost.cost = 1.0f + n_node->influence;
			adjacent->push_back(cost);
		}
	}
}

void InitAI( AITestMem* aiMem, float screen_x, float screen_y, float edgeBuffer ) {
	aiMap.entities = (AI_Entity*)&aiMem->entities;
	aiMap.mapXMax = (screen_x / 2.0f) - edgeBuffer;
	aiMap.mapYMax = (screen_y / 2.0f) - edgeBuffer;
	aiMap.mapXMin = (-screen_x / 2.0f) + 2.0f * edgeBuffer;
	aiMap.mapYMin = (-screen_y / 2.0f) + 2.0f * edgeBuffer;
	float mapXRange = aiMap.mapXMax - aiMap.mapXMin;
	float mapYRange = aiMap.mapYMax - aiMap.mapYMin;
	aiMap.mapXStep = mapXRange / (float)MAP_HORZ_SEGS;
	aiMap.mapYStep = mapYRange / (float)MAP_VERT_SEGS;
	for( int y = 0; y < MAP_VERT_SEGS; y++ ) {
		for( int x = 0; x < MAP_HORZ_SEGS; x++ ) {
			MapNode* node = &aiMap.allNodes[ y ][ x ];
			node->location.x = aiMap.mapXMin + (aiMap.mapXStep * (float)x);
			node->location.y = aiMap.mapYMin + (aiMap.mapYStep * (float)y);
			node->xIndex = x;
			node->yIndex = y;
			node->influence = 0.0f;
			node->neighborCount = 0;
			for( int n_index = 0; n_index < 9; n_index++ ) {
				int n_yIndex = node->yIndex + (n_index / 3) - 1;
				int n_xIndex = node->xIndex + (n_index % 3) - 1;

				if(n_yIndex == node->yIndex && n_xIndex == node->xIndex ) continue;

				if(n_yIndex >= 0 && n_yIndex < MAP_VERT_SEGS && n_xIndex >= 0 && n_xIndex < MAP_HORZ_SEGS) {
					MapNode* n_node = &aiMap.allNodes[n_yIndex][n_xIndex];
					node->neighbors[node->neighborCount++] = n_node;
				}
			}
		}
	}

	aiMap.xBoundHigh = MAP_HORZ_SEGS;
	aiMap.xBoundLow = 0;
	aiMap.yBoundHigh = MAP_VERT_SEGS;
	aiMap.yBoundLow = 0;

	aiMem->entities[0].p = { -120.0f, -100.0f };
	aiMem->entities[1].p = { -200.0f, -60.0f };
	aiMem->entities[2].p = { -180.0f, 20.0f };
	aiMem->entities[3].p = { -150.0f, 110.0f };
}

void UpdateAI( AITestMem* aiMem ) {

	for( int i = 0; i < 4; i++ ) {
		aiMem->entities[i].pathCount = 0;
	}

	for( int i = 0; i < MAP_VERT_SEGS; i++) {
		for( int j = 0; j < MAP_HORZ_SEGS; j++ ) {
			aiMap.allNodes[i][j].influence = 0.0f;
		}
	}

	Vec2 target = { 200.0f, 0.0f };

	uint8 nextEntityIndex = 0;
	AI_Entity* orderedEntites[4];
	while( nextEntityIndex < 4 ) {
		uint8 insertIndex = 0;
		float closestDist = 10000.0f;
		for( int i = 0; i < 4; i++ ) {
			AI_Entity* e = &aiMem->entities[i];

			bool alreadyAdded = false;
			for( int j = 0; j < nextEntityIndex; j++ ) {
				if( e == orderedEntites[j] ) {
					alreadyAdded = true;
					break;
				}
			}

			if( !alreadyAdded ) {
				float xDiff = e->p.x - target.x;
				float yDiff = e->p.y - target.y;
				float thisDistance = sqrtf( xDiff * xDiff + yDiff * yDiff );

				if( thisDistance < closestDist ){
					closestDist = thisDistance;
					insertIndex = i;
				}
			}
		}

		orderedEntites[nextEntityIndex++] = &aiMem->entities[insertIndex];
	}

	int yIndex = (int)((target.y - aiMap.mapYMin) / aiMap.mapYStep);
	int xIndex = (int)((target.x - aiMap.mapXMin) / aiMap.mapXStep);
	MapNode* targetNode = &aiMap.allNodes[yIndex][xIndex];

	for( int i = 0; i < 4; i++ ) {
		AI_Entity* e = orderedEntites[i];
		yIndex = (int)((e->p.y - aiMap.mapYMin) / aiMap.mapYStep);
		xIndex = (int)((e->p.x - aiMap.mapXMin) / aiMap.mapXStep);
		MapNode* startNode = &aiMap.allNodes[yIndex][xIndex];

		MicroPather pather(&aiMap);
		float totalCost = 0.0f;
		MP_VECTOR< void* > myPath;
		pather.Solve( (void*)startNode, (void*)targetNode, &myPath, &totalCost );

		Vec3 vecTowards = { targetNode->location.x - e->p.x, targetNode->location.y - e->p.y, 0.0f };
		Normalize( &vecTowards );

		e->pathCount = 1;
		e->currentPath[0] = e->p;
		for( int pathCopy = 0; pathCopy < myPath.size() && pathCopy < 32; pathCopy++ ) {
			MapNode* node = (MapNode*)myPath[pathCopy];
			Vec3 vecToNode = { node->location.x - e->p.x, node->location.y - e->p.y, 0.0f };
			Normalize( &vecToNode );

			float doot = Dot( vecTowards, vecToNode );
	        if ( doot > 0.33f ) {
	        	e->currentPath[e->pathCount] = node->location;
	        	e->pathCount++;

	        	//Floodfile influence
	        	const float baseInfluence = 50.0f;
	        	node->influence = baseInfluence;

	        	for( uint8 baseNIndex = 0; baseNIndex < node->neighborCount; baseNIndex++ ) {
	        		const float gen1Influence = baseInfluence * 0.75f;
	        		MapNode* g1Node = node->neighbors[baseNIndex];
	        		if( g1Node->influence != baseInfluence ){
	        			node->neighbors[baseNIndex]->influence = gen1Influence;
	        		}

	        		for( uint8 g1_N_Index = 0; g1_N_Index < g1Node->neighborCount; g1_N_Index++ ){
	        			const float gen2Influence = baseInfluence * 0.5f;
	        			MapNode* g1Neighbor = g1Node->neighbors[g1_N_Index];

	        			int xDiff = node->xIndex - g1Neighbor->xIndex;
	        			int yDiff = node->yIndex - g1Neighbor->yIndex;

	        			if( std::abs(xDiff) == 2 || std::abs(yDiff) == 2 ) {
	        				if(g1Neighbor->influence < gen2Influence)
	        					g1Neighbor->influence = gen2Influence;
	        			}
	        		}
	        	}
			}
		}

		Vec2 towardsNextPoint;
		if( e->pathCount > 1 ){
			towardsNextPoint = { e->currentPath[1].x - e->p.x, e->currentPath[1].y - e->p.y };
		} else {
			towardsNextPoint = { target.x - e->p.x, target.y - e->p.y };
		}

		const float moveSpeed = 0.5f;
		float inv_len = 1.0f / sqrtf(towardsNextPoint.x * towardsNextPoint.x + towardsNextPoint.y * towardsNextPoint.y);
		towardsNextPoint.x *= inv_len * moveSpeed;
		towardsNextPoint.y *= inv_len * moveSpeed;
		e->p.x += towardsNextPoint.x;
		e->p.y += towardsNextPoint.y;
	}
}

void RenderAI( AITestMem* aiMem ) {
	for( int y = 0; y < MAP_VERT_SEGS; y++ ){
		for( int x = 0; x < MAP_HORZ_SEGS; x++ ) {
			Vec2 p = aiMap.allNodes[y][x].location;
			float influence = aiMap.allNodes[y][x].influence / 50.0f;
			RenderCircle( {p.x, p.y, 0.0f}, 6.0f, {influence, influence, influence} );
		}
	}

	for( int aiIndex = 0; aiIndex < 4; aiIndex++ ) {
		AI_Entity* e = &aiMem->entities[aiIndex];

		RenderCircle({e->p.x, e->p.y, 0.0f}, 8.0f, {1.0f, 0.8f, 0.8f});

		float points[3 * 33];
		uint32 indecies[33];
		points[0] = e->p.x; points[1] = e->p.y; points[2] = 0.0f;
		indecies[0] = 0;
		for( int count = 1; count < e->pathCount; count++ ) {
			points[ 3 * count ] = e->currentPath[count].x;
			points[ 3 * count + 1 ] = e->currentPath[count].y;
			points[ 3 * count + 2 ] = 0.0f;

			indecies[count] = count;
		}

		RenderLineStrip( (float*)&points, (uint32*)&indecies, e->pathCount );
	}
}