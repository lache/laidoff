#include <stdlib.h>
#include <string.h>
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "nav.h"
#include "file.h"
#include "lwlog.h"
#include "lwmacro.h"

#define MAX_PATHQUERY_POLYS (256)

class LWNAV {
public:
	dtNavMesh* nav_mesh;
	dtNavMeshQuery* nav_query;
	dtPolyRef start_ref;
	dtPolyRef end_ref;
	float poly_pick_ext[3];
	dtQueryFilter filter;
	dtPolyRef polys[MAX_PATHQUERY_POLYS];
	int n_polys;
};

enum SamplePolyAreas {
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
};
enum SamplePolyFlags {
	SAMPLE_POLYFLAGS_WALK = 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM = 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR = 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP = 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED = 0x10,		// Disabled polygon
	SAMPLE_POLYFLAGS_ALL = 0xffff	// All abilities.
};

// Returns a random number [0..1]
static float frand() {
	//	return ((float)(rand() & 0xffff)/(float)0xffff);
	return (float)rand() / (float)RAND_MAX;
}

void* load_nav(const char* filename) {
	auto nav = new LWNAV();
	nav->nav_mesh = dtAllocNavMesh();
	
	size_t size = 0;
	char* d = create_binary_from_file(filename, &size);

	dtStatus status;
	status = nav->nav_mesh->init(reinterpret_cast<unsigned char*>(d), size, DT_TILE_FREE_DATA);
	if (dtStatusFailed(status)) {
		LOGE("nav->nav_mesh->init() error");
	}

	nav->nav_query = dtAllocNavMeshQuery();

	status = nav->nav_query->init(nav->nav_mesh, 2048);
	if (dtStatusFailed(status)) {
		LOGE("nav->nav_query->init() error");
	}

	nav->start_ref = 0;
	nav->end_ref = 0;

	nav->poly_pick_ext[0] = 2;
	nav->poly_pick_ext[1] = 4;
	nav->poly_pick_ext[2] = 2;

	nav->filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
	nav->filter.setExcludeFlags(0);
	nav->filter.setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);
	nav->filter.setAreaCost(SAMPLE_POLYAREA_WATER, 10.0f);
	nav->filter.setAreaCost(SAMPLE_POLYAREA_ROAD, 1.0f);
	nav->filter.setAreaCost(SAMPLE_POLYAREA_DOOR, 1.0f);
	nav->filter.setAreaCost(SAMPLE_POLYAREA_GRASS, 2.0f);
	nav->filter.setAreaCost(SAMPLE_POLYAREA_JUMP, 1.5f);

	
	return nav;
}

void set_random_start_end_pos(void* n, LWPATHQUERY* q) {
	LWNAV* nav = reinterpret_cast<LWNAV*>(n);
	dtStatus status = nav->nav_query->findRandomPoint(&nav->filter, frand, &nav->start_ref, q->spos);
	if (dtStatusFailed(status)) {
		LOGE("nav->nav_query->findRandomPoint() error");
	}

	status = nav->nav_query->findRandomPoint(&nav->filter, frand, &nav->end_ref, q->epos);
	if (dtStatusFailed(status)) {
		LOGE("nav->nav_query->findRandomPoint() error");
	}
}

void unload_nav(void* n) {
	LWNAV* nav = reinterpret_cast<LWNAV*>(n);
	dtFreeNavMeshQuery(nav->nav_query);
	dtFreeNavMesh(nav->nav_mesh);
	delete nav;
}

inline bool inRange(const float* v1, const float* v2, const float r, const float h) {
	const float dx = v2[0] - v1[0];
	const float dy = v2[1] - v1[1];
	const float dz = v2[2] - v1[2];
	return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
}

static bool getSteerTarget(dtNavMeshQuery* navQuery, const float* startPos, const float* endPos,
	const float minTargetDist,
	const dtPolyRef* path, const int pathSize,
	float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
	float* outPoints = 0, int* outPointCount = 0) {
	// Find steer target.
	static const int MAX_STEER_POINTS = 3;
	float steerPath[MAX_STEER_POINTS * 3];
	unsigned char steerPathFlags[MAX_STEER_POINTS];
	dtPolyRef steerPathPolys[MAX_STEER_POINTS];
	int nsteerPath = 0;
	navQuery->findStraightPath(startPos, endPos, path, pathSize,
		steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);
	if (!nsteerPath)
		return false;

	if (outPoints && outPointCount) {
		*outPointCount = nsteerPath;
		for (int i = 0; i < nsteerPath; ++i)
			dtVcopy(&outPoints[i * 3], &steerPath[i * 3]);
	}


	// Find vertex far enough to steer to.
	int ns = 0;
	while (ns < nsteerPath) {
		// Stop at Off-Mesh link or when point is further than slop away.
		if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
			!inRange(&steerPath[ns * 3], startPos, minTargetDist, 1000.0f))
			break;
		ns++;
	}
	// Failed to find good point to steer to.
	if (ns >= nsteerPath)
		return false;

	dtVcopy(steerPos, &steerPath[ns * 3]);
	steerPos[1] = startPos[1];
	steerPosFlag = steerPathFlags[ns];
	steerPosRef = steerPathPolys[ns];

	return true;
}

static int fixupCorridor(dtPolyRef* path, const int npath, const int maxPath,
	const dtPolyRef* visited, const int nvisited) {
	int furthestPath = -1;
	int furthestVisited = -1;

	// Find furthest common polygon.
	for (int i = npath - 1; i >= 0; --i) {
		bool found = false;
		for (int j = nvisited - 1; j >= 0; --j) {
			if (path[i] == visited[j]) {
				furthestPath = i;
				furthestVisited = j;
				found = true;
			}
		}
		if (found)
			break;
	}

	// If no intersection found just return current path. 
	if (furthestPath == -1 || furthestVisited == -1)
		return npath;

	// Concatenate paths.	

	// Adjust beginning of the buffer to include the visited.
	const int req = nvisited - furthestVisited;
	const int orig = LWMIN(furthestPath + 1, npath);
	int size = LWMAX(0, npath - orig);
	if (req + size > maxPath)
		size = maxPath - req;
	if (size)
		memmove(path + req, path + orig, size * sizeof(dtPolyRef));

	// Store visited
	for (int i = 0; i < req; ++i)
		path[i] = visited[(nvisited - 1) - i];

	return req + size;
}


// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary, 
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
static int fixupShortcuts(dtPolyRef* path, int npath, dtNavMeshQuery* navQuery) {
	if (npath < 3)
		return npath;

	// Get connected polygons
	static const int maxNeis = 16;
	dtPolyRef neis[maxNeis];
	int nneis = 0;

	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	if (dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
		return npath;

	for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next) {
		const dtLink* link = &tile->links[k];
		if (link->ref != 0) {
			if (nneis < maxNeis)
				neis[nneis++] = link->ref;
		}
	}

	// If any of the neighbour polygons is within the next few polygons
	// in the path, short cut to that polygon directly.
	static const int maxLookAhead = 6;
	int cut = 0;
	for (int i = dtMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--) {
		for (int j = 0; j < nneis; j++) {
			if (path[i] == neis[j]) {
				cut = i;
				break;
			}
		}
	}
	if (cut > 1) {
		int offset = cut - 1;
		npath -= offset;
		for (int i = 1; i < npath; i++)
			path[i] = path[i + offset];
	}

	return npath;
}

void nav_query(void* n, LWPATHQUERY* q) {
	LWNAV* nav = reinterpret_cast<LWNAV*>(n);

	nav->nav_query->findNearestPoly(q->spos, nav->poly_pick_ext, &nav->filter, &nav->start_ref, 0);
	nav->nav_query->findNearestPoly(q->epos, nav->poly_pick_ext, &nav->filter, &nav->end_ref, 0);
	nav->nav_query->findPath(nav->start_ref, nav->end_ref, q->spos, q->epos, &nav->filter, nav->polys, &nav->n_polys, MAX_PATHQUERY_POLYS);

	q->n_smooth_path = 0;

	if (nav->n_polys) {
		// Iterate over the path to find smooth path on the detail mesh surface.
		dtPolyRef polys[MAX_PATHQUERY_POLYS];
		memcpy(polys, nav->polys, sizeof(dtPolyRef)*nav->n_polys);
		int npolys = nav->n_polys;

		float iterPos[3], targetPos[3];
		nav->nav_query->closestPointOnPoly(nav->start_ref, q->spos, iterPos, 0);
		nav->nav_query->closestPointOnPoly(polys[npolys - 1], q->epos, targetPos, 0);

		static const float STEP_SIZE = 0.5f;
		static const float SLOP = 0.01f;

		q->n_smooth_path = 0;

		dtVcopy(&q->smooth_path[q->n_smooth_path * 3], iterPos);
		q->n_smooth_path++;

		// Move towards target a small advancement at a time until target reached or
		// when ran out of memory to store the path.
		while (npolys && q->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
			// Find location to steer towards.
			float steerPos[3];
			unsigned char steerPosFlag;
			dtPolyRef steerPosRef;

			if (!getSteerTarget(nav->nav_query, iterPos, targetPos, SLOP,
				polys, npolys, steerPos, steerPosFlag, steerPosRef))
				break;

			bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
			bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

			// Find movement delta.
			float delta[3], len;
			dtVsub(delta, steerPos, iterPos);
			len = dtMathSqrtf(dtVdot(delta, delta));
			// If the steer target is end of path or off-mesh link, do not move past the location.
			if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
				len = 1;
			else
				len = STEP_SIZE / len;
			float moveTgt[3];
			dtVmad(moveTgt, iterPos, delta, len);

			// Move
			float result[3];
			dtPolyRef visited[16];
			int nvisited = 0;
			nav->nav_query->moveAlongSurface(polys[0], iterPos, moveTgt, &nav->filter,
				result, visited, &nvisited, 16);

			npolys = fixupCorridor(polys, npolys, MAX_PATHQUERY_POLYS, visited, nvisited);
			npolys = fixupShortcuts(polys, npolys, nav->nav_query);

			float h = 0;
			nav->nav_query->getPolyHeight(polys[0], result, &h);
			result[1] = h;
			dtVcopy(iterPos, result);

			// Handle end of path and off-mesh links when close enough.
			if (endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f)) {
				// Reached end of path.
				dtVcopy(iterPos, targetPos);
				if (q->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
					dtVcopy(&q->smooth_path[q->n_smooth_path * 3], iterPos);
					q->n_smooth_path++;
				}
				break;
			} else if (offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f)) {
				// Reached off-mesh connection.
				float startPos[3], endPos[3];

				// Advance the path up to and over the off-mesh connection.
				dtPolyRef prevRef = 0, polyRef = polys[0];
				int npos = 0;
				while (npos < npolys && polyRef != steerPosRef) {
					prevRef = polyRef;
					polyRef = polys[npos];
					npos++;
				}
				for (int i = npos; i < npolys; ++i)
					polys[i - npos] = polys[i];
				npolys -= npos;

				// Handle the connection.
				dtStatus status = nav->nav_mesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
				if (dtStatusSucceed(status)) {
					if (q->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
						dtVcopy(&q->smooth_path[q->n_smooth_path * 3], startPos);
						q->n_smooth_path++;
						// Hack to make the dotted path not visible during off-mesh connection.
						if (q->n_smooth_path & 1) {
							dtVcopy(&q->smooth_path[q->n_smooth_path * 3], startPos);
							q->n_smooth_path++;
						}
					}
					// Move position at the other side of the off-mesh link.
					dtVcopy(iterPos, endPos);
					float eh = 0.0f;
					nav->nav_query->getPolyHeight(polys[0], iterPos, &eh);
					iterPos[1] = eh;
				}
			}

			// Store results.
			if (q->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
				dtVcopy(&q->smooth_path[q->n_smooth_path * 3], iterPos);
				q->n_smooth_path++;
			}
		}
	}
}
