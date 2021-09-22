#pragma once

#include "Recast/SampleInterfaces.h"
#include "Utils/Buffer.h"
#include <vector>

class Scene;

class dtCrowd;
class dtNavMeshQuery;
class dtNavMesh;
class dtTileCache;

class NavMesh {
public:
	NavMesh();	// Allocates navQuery data and crowd memory
	~NavMesh(); // Releases memory

	bool Build(Scene* scene);		 // Generates the navMesh from Scene Meshes (Vertices and triangles of MeshRenderer), and saves the data in navData and navDataSize. Also inits navMesh, navQuery and crowd.
	void DrawGizmos(Scene* scene);	 // Drawas the Bounding Box and the NavMesh
	void Load(Buffer<char>& buffer); // Loads NavMesh from buffer and Inits data
	void CleanUp();					 // Releases memory
	Buffer<char> Save();

	bool IsGenerated();				   // Returns true if navMesh is valid
	dtCrowd* GetCrowd();			   // Returns crowd
	dtNavMeshQuery* GetNavMeshQuery(); // Returns navQuery
	dtNavMesh* GetNavMesh();		   // Returns navMesh
	dtTileCache* GetTileCache();	   // Returns tileCache

public:
	enum DrawMode {
		DRAWMODE_NAVMESH,
		DRAWMODE_NAVMESH_TRANS,
		DRAWMODE_NAVMESH_BVTREE,
		DRAWMODE_NAVMESH_NODES,
		DRAWMODE_NAVMESH_PORTALS,
		DRAWMODE_NAVMESH_INVIS,
		DRAWMODE_MESH,
		DRAWMODE_CACHE_BOUNDS,
		MAX_DRAWMODE
	};

	// NAV MESH BUILD PARAMETERS
	// AGENT
	float agentHeight = 2.0f;
	float agentRadius = 0.5f;
	float agentMaxClimb = 0.9f;
	float agentMaxSlope = 45.0f;

	// RASTERIZATION
	float cellSize = 0.30f;
	float cellHeight = 0.20f;

	// REGION
	int regionMinSize = 8;
	int regionMergeSize = 20;

	// PARTITIONING
	int partitionType = 0;

	// POLYGONIZATION
	int edgeMaxLen = 12;
	float edgeMaxError = 1.3f;
	int vertsPerPoly = 6;

	// DETAIL MESH
	int detailSampleDist = 6;
	int detailSampleMaxError = 1;

	// INTERMEDIATE RESULTS
	bool keepInterResults = false;

	// TILING
	int tileSize = 48;
	int maxTiles = 0;
	int maxPolysPerTile = 0;

	// DRAW MODE
	DrawMode drawMode = DRAWMODE_NAVMESH;

private:
	void InitCrowd(); // Inits crowd with MAX_AGENTS

private:
	BuildContext* ctx = nullptr;

	class dtNavMesh* navMesh = nullptr;
	class dtNavMeshQuery* navQuery = nullptr;
	class dtCrowd* crowd = nullptr;

	class dtTileCache* tileCache = nullptr;

	struct LinearAllocator* talloc = nullptr;
	struct FastLZCompressor* tcomp = nullptr;
	struct MeshProcess* tmproc = nullptr;

	unsigned char navMeshDrawFlags = 0;
};
