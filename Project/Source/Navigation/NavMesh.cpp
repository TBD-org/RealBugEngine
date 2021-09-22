#include "NavMesh.h"

#include "Application.h"
#include "Modules/ModuleDebugDraw.h"
#include "Modules/ModuleCamera.h"
#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentAgent.h"
#include "Components/ComponentObstacle.h"

#include "Scene.h"

#include "Utils/Logging.h"

#include "Recast/Recast.h"
#include "Recast/RecastAlloc.h"
#include "Recast/RecastAssert.h"
#include "Detour/DetourStatus.h"
#include "Detour/DetourNavMesh.h"
#include "Detour/DetourNavMeshBuilder.h"
#include "Detour/DetourNavMeshQuery.h"
#include "Detour/DetourCommon.h"
#include "Detour/InputGeom.h"
#include "Detour/DetourAssert.h"
#include "DetourTileCache/DetourTileCache.h"
#include "DetourTileCache/DetourTileCacheBuilder.h"
#include "DetourCrowd/DetourCrowd.h"
#include "DebugUtils/RecastDebugDraw.h"
#include "DebugUtils/DetourDebugDraw.h"
#include "fastlz/fastlz.h"
#include "GL/glew.h"

#include "Utils/Leaks.h"

#define MAX_AGENTS 128

// This value specifies how many layers (or "floors") each navmesh tile is expected to have.
static const int EXPECTED_LAYERS_PER_TILE = 4;

/// These are just sample areas to use consistent values across the samples.
/// The use should specify these base on his needs.
enum SamplePolyAreas {
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
};

enum SamplePolyFlags {
	SAMPLE_POLYFLAGS_WALK = 0x01,	  // Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM = 0x02,	  // Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR = 0x04,	  // Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP = 0x08,	  // Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED = 0x10, // Disabled polygon
	SAMPLE_POLYFLAGS_ALL = 0xffff	  // All abilities.
};

enum SamplePartitionType {
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS,
};

static const int MAX_LAYERS = 32;

struct TileCacheData {
	unsigned char* data;
	int dataSize;
};

struct FastLZCompressor : public dtTileCacheCompressor {
	virtual int maxCompressedSize(const int bufferSize) override {
		return (int) (bufferSize * 1.05f);
	}

	virtual dtStatus compress(const unsigned char* buffer, const int bufferSize, unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize) override {
		*compressedSize = fastlz_compress((const void* const) buffer, bufferSize, compressed);
		return DT_SUCCESS;
	}

	virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize, unsigned char* buffer, const int maxBufferSize, int* bufferSize) override {
		*bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
		return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
	}
};

struct LinearAllocator : public dtTileCacheAlloc {
	unsigned char* buffer;
	int capacity;
	int top;
	int high;

	LinearAllocator(const int cap)
		: buffer(0)
		, capacity(0)
		, top(0)
		, high(0) {
		resize(cap);
	}

	virtual ~LinearAllocator() override {
		dtFree(buffer);
	}

	void resize(const int cap) {
		if (buffer) dtFree(buffer);
		buffer = (unsigned char*) dtAlloc(cap, DT_ALLOC_PERM);
		capacity = cap;
	}

	virtual void reset() override {
		high = dtMax(high, top);
		top = 0;
	}

	virtual void* alloc(const size_t size) override {
		if (!buffer)
			return 0;
		if (top + size > capacity)
			return 0;
		unsigned char* mem = &buffer[top];
		top += size;
		return mem;
	}

	virtual void free(void* /*ptr*/) override {
		// Empty
	}
};

struct MeshProcess : public dtTileCacheMeshProcess {
	InputGeom* m_geom;

	inline MeshProcess()
		: m_geom(0) {
	}

	inline void init(InputGeom* geom) {
		m_geom = geom;
	}

	virtual void process(struct dtNavMeshCreateParams* params, unsigned char* polyAreas, unsigned short* polyFlags) override {
		// Update poly flags from areas.
		for (int i = 0; i < params->polyCount; ++i) {
			if (polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
				polyAreas[i] = SAMPLE_POLYAREA_GROUND;

			if (polyAreas[i] == SAMPLE_POLYAREA_GROUND || polyAreas[i] == SAMPLE_POLYAREA_GRASS || polyAreas[i] == SAMPLE_POLYAREA_ROAD) {
				polyFlags[i] = SAMPLE_POLYFLAGS_WALK;
			} else if (polyAreas[i] == SAMPLE_POLYAREA_WATER) {
				polyFlags[i] = SAMPLE_POLYFLAGS_SWIM;
			} else if (polyAreas[i] == SAMPLE_POLYAREA_DOOR) {
				polyFlags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}

		// Pass in off-mesh connections.
		if (m_geom) {
			params->offMeshConVerts = m_geom->getOffMeshConnectionVerts();
			params->offMeshConRad = m_geom->getOffMeshConnectionRads();
			params->offMeshConDir = m_geom->getOffMeshConnectionDirs();
			params->offMeshConAreas = m_geom->getOffMeshConnectionAreas();
			params->offMeshConFlags = m_geom->getOffMeshConnectionFlags();
			params->offMeshConUserID = m_geom->getOffMeshConnectionId();
			params->offMeshConCount = m_geom->getOffMeshConnectionCount();
		}
	}
};

struct RasterizationContext {
	RasterizationContext()
		: solid(0)
		, triareas(0)
		, lset(0)
		, chf(0)
		, ntiles(0) {
		memset(tiles, 0, sizeof(TileCacheData) * MAX_LAYERS);
	}

	~RasterizationContext() {
		rcFreeHeightField(solid);
		delete[] triareas;
		rcFreeHeightfieldLayerSet(lset);
		rcFreeCompactHeightfield(chf);
		for (int i = 0; i < MAX_LAYERS; ++i) {
			dtFree(tiles[i].data);
			tiles[i].data = 0;
		}
	}

	rcHeightfield* solid;
	unsigned char* triareas;
	rcHeightfieldLayerSet* lset;
	rcCompactHeightfield* chf;
	TileCacheData tiles[MAX_LAYERS];
	int ntiles;
};

static int RasterizeTileLayers(float* verts, int nVerts, int nTris, BuildContext* ctx, rcChunkyTriMesh* chunkyMesh, const int tx, const int ty, const rcConfig& cfg, TileCacheData* tiles, const int maxTiles) {
	FastLZCompressor comp;
	RasterizationContext rc;

	// Tile bounds.
	const float tcs = cfg.tileSize * cfg.cs;

	rcConfig tcfg;
	memcpy(&tcfg, &cfg, sizeof(tcfg));

	tcfg.bmin[0] = cfg.bmin[0] + tx * tcs;
	tcfg.bmin[1] = cfg.bmin[1];
	tcfg.bmin[2] = cfg.bmin[2] + ty * tcs;
	tcfg.bmax[0] = cfg.bmin[0] + (tx + 1) * tcs;
	tcfg.bmax[1] = cfg.bmax[1];
	tcfg.bmax[2] = cfg.bmin[2] + (ty + 1) * tcs;
	tcfg.bmin[0] -= tcfg.borderSize * tcfg.cs;
	tcfg.bmin[2] -= tcfg.borderSize * tcfg.cs;
	tcfg.bmax[0] += tcfg.borderSize * tcfg.cs;
	tcfg.bmax[2] += tcfg.borderSize * tcfg.cs;

	// Allocate voxel heightfield where we rasterize our input data to.
	rc.solid = rcAllocHeightfield();
	if (!rc.solid) {
		LOG("buildNavigation: Out of memory 'solid'.");
		return 0;
	}
	if (!rcCreateHeightfield(ctx, *rc.solid, tcfg.width, tcfg.height, tcfg.bmin, tcfg.bmax, tcfg.cs, tcfg.ch)) {
		LOG("buildNavigation: Could not create solid heightfield.");
		return 0;
	}

	// Allocate array that can hold triangle flags.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	rc.triareas = new unsigned char[nTris];
	if (!rc.triareas) {
		LOG("buildNavigation: Out of memory 'm_triareas' (%d).", nTris);
		return 0;
	}

	float tbmin[2], tbmax[2];
	tbmin[0] = tcfg.bmin[0];
	tbmin[1] = tcfg.bmin[2];
	tbmax[0] = tcfg.bmax[0];
	tbmax[1] = tcfg.bmax[2];
	int cid[512]; // TODO: Make grow when returning too many items.
	const int ncid = rcGetChunksOverlappingRect(chunkyMesh, tbmin, tbmax, cid, 512);
	if (!ncid) {
		return 0; // empty
	}

	for (int i = 0; i < ncid; ++i) {
		const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
		const int* tris = &chunkyMesh->tris[node.i * 3];
		const int ntris = node.n;

		memset(rc.triareas, 0, ntris * sizeof(unsigned char));
		rcMarkWalkableTriangles(ctx, tcfg.walkableSlopeAngle, verts, nVerts, tris, ntris, rc.triareas);

		if (!rcRasterizeTriangles(ctx, verts, nVerts, tris, rc.triareas, ntris, *rc.solid, tcfg.walkableClimb))
			return 0;
	}

	// Once all geometry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(ctx, tcfg.walkableClimb, *rc.solid);
	rcFilterLedgeSpans(ctx, tcfg.walkableHeight, tcfg.walkableClimb, *rc.solid);
	rcFilterWalkableLowHeightSpans(ctx, tcfg.walkableHeight, *rc.solid);

	rc.chf = rcAllocCompactHeightfield();
	if (!rc.chf) {
		LOG("buildNavigation: Out of memory 'chf'.");
		return 0;
	}
	if (!rcBuildCompactHeightfield(ctx, tcfg.walkableHeight, tcfg.walkableClimb, *rc.solid, *rc.chf)) {
		LOG("buildNavigation: Could not build compact data.");
		return 0;
	}

	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(ctx, tcfg.walkableRadius, *rc.chf)) {
		LOG("buildNavigation: Could not erode.");
		return 0;
	}

	//// (Optional) Mark areas.
	//const ConvexVolume* vols = geom->getConvexVolumes();
	//for (int i = 0; i < geom->getConvexVolumeCount(); ++i) {
	//	rcMarkConvexPolyArea(ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char) vols[i].area, *rc.chf);
	//}

	rc.lset = rcAllocHeightfieldLayerSet();
	if (!rc.lset) {
		LOG("buildNavigation: Out of memory 'lset'.");
		return 0;
	}
	if (!rcBuildHeightfieldLayers(ctx, *rc.chf, tcfg.borderSize, tcfg.walkableHeight, *rc.lset)) {
		LOG("buildNavigation: Could not build heighfield layers.");
		return 0;
	}

	rc.ntiles = 0;
	for (int i = 0; i < rcMin(rc.lset->nlayers, MAX_LAYERS); ++i) {
		TileCacheData* tile = &rc.tiles[rc.ntiles++];
		const rcHeightfieldLayer* layer = &rc.lset->layers[i];

		// Store header
		dtTileCacheLayerHeader header;
		header.magic = DT_TILECACHE_MAGIC;
		header.version = DT_TILECACHE_VERSION;

		// Tile layer location in the navmesh.
		header.tx = tx;
		header.ty = ty;
		header.tlayer = i;
		dtVcopy(header.bmin, layer->bmin);
		dtVcopy(header.bmax, layer->bmax);

		// Tile info.
		header.width = (unsigned char) layer->width;
		header.height = (unsigned char) layer->height;
		header.minx = (unsigned char) layer->minx;
		header.maxx = (unsigned char) layer->maxx;
		header.miny = (unsigned char) layer->miny;
		header.maxy = (unsigned char) layer->maxy;
		header.hmin = (unsigned short) layer->hmin;
		header.hmax = (unsigned short) layer->hmax;

		dtStatus status = dtBuildTileCacheLayer(&comp, &header, layer->heights, layer->areas, layer->cons, &tile->data, &tile->dataSize);
		if (dtStatusFailed(status)) {
			return 0;
		}
	}

	// Transfer ownsership of tile data from build context to the caller.
	int n = 0;
	for (int i = 0; i < rcMin(rc.ntiles, maxTiles); ++i) {
		tiles[n++] = rc.tiles[i];
		rc.tiles[i].data = 0;
		rc.tiles[i].dataSize = 0;
	}

	return n;
}

void DrawTiles(duDebugDraw* dd, dtTileCache* tc) {
	unsigned int fcol[6];
	float bmin[3], bmax[3];

	for (int i = 0; i < tc->getTileCount(); ++i) {
		const dtCompressedTile* tile = tc->getTile(i);
		if (!tile->header) continue;

		tc->calcTightTileBounds(tile->header, bmin, bmax);

		const unsigned int col = duIntToCol(i, 64);
		duCalcBoxColors(fcol, col, col);
		duDebugDrawBox(dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], fcol);
	}

	for (int i = 0; i < tc->getTileCount(); ++i) {
		const dtCompressedTile* tile = tc->getTile(i);
		if (!tile->header) continue;

		tc->calcTightTileBounds(tile->header, bmin, bmax);

		const unsigned int col = duIntToCol(i, 255);
		const float pad = tc->getParams()->cs * 0.1f;
		duDebugDrawBoxWire(dd, bmin[0] - pad, bmin[1] - pad, bmin[2] - pad, bmax[0] + pad, bmax[1] + pad, bmax[2] + pad, col, 2.0f);
	}
}

void DrawObstacles(duDebugDraw* dd, const dtTileCache* tc) {
	// Draw obstacles
	for (int i = 0; i < tc->getObstacleCount(); ++i) {
		const dtTileCacheObstacle* ob = tc->getObstacle(i);
		if (ob->state == DT_OBSTACLE_EMPTY) continue;
		if (!ob->mustBeDrawnGizmo) continue;
		float bmin[3], bmax[3];
		tc->getObstacleBounds(ob, bmin, bmax);

		unsigned int col = 0;
		if (ob->state == DT_OBSTACLE_PROCESSING) {
			col = duRGBA(255, 255, 0, 128);
		} else if (ob->state == DT_OBSTACLE_PROCESSED) {
			col = duRGBA(255, 192, 0, 192);
		} else if (ob->state == DT_OBSTACLE_REMOVING) {
			col = duRGBA(220, 0, 0, 128);
		}

		if (ob->type == ObstacleType::DT_OBSTACLE_CYLINDER) {
			duDebugDrawCylinder(dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], col);
			duDebugDrawCylinderWire(dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], duDarkenCol(col), 2);
		} else {
			//AABB localAABB = {{bmin[0], bmin[1], bmin[2]}, {bmax[0], bmax[1], bmax[2]}};
			//OBB worldOBB = OBB(localAABB);
			////float3 eulerAngle = float3(0, ob->orientedBox.yRadian, 0);
			//worldOBB.Transform(Quat::FromEulerXYZ(0, ob->orientedBox.yRadian, 0));
			//AABB worldAABB = worldOBB.MinimalEnclosingAABB();
			//bmin[0] = worldAABB.minPoint[0];
			//bmin[1] = worldAABB.minPoint[1];
			//bmin[2] = worldAABB.minPoint[2];

			//bmax[0] = worldAABB.maxPoint[0];
			//bmax[1] = worldAABB.maxPoint[1];
			//bmax[2] = worldAABB.maxPoint[2];
			// TODO: PROPERLY SHOW THE DEBUG OBB. In function getObstacleBounds there are magic numbers multiplying the ABB and not applying a rotation. That's source code from the library.

			const unsigned int* colConst = &col;
			duDebugDrawBox(dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], colConst);
			duDebugDrawBoxWire(dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], duDarkenCol(col), 2);
		}
	}
}

NavMesh::NavMesh() {
	navMeshDrawFlags = DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST;
	navQuery = dtAllocNavMeshQuery();
	crowd = dtAllocCrowd();

	ctx = new BuildContext();
	talloc = new LinearAllocator(32000);
	tcomp = new FastLZCompressor;
	tmproc = new MeshProcess;
}

NavMesh::~NavMesh() {
	CleanUp();

	dtFreeCrowd(crowd);
	crowd = nullptr;
	dtFreeNavMeshQuery(navQuery);
	navQuery = nullptr;
	RELEASE(ctx);
}

bool NavMesh::Build(Scene* scene) {
	CleanUp();

	std::vector<float> verts = scene->GetVertices();
	std::vector<int> tris = scene->GetTriangles();
	std::vector<float> normals = scene->GetNormals();

	unsigned ntris = tris.size() / 3;

	if (verts.size() == 0) {
		LOG("Building navigation:");
		LOG("There's no mesh to build");
		return false;
	}

	talloc = new LinearAllocator(32000);
	tcomp = new FastLZCompressor;
	tmproc = new MeshProcess;

	float bmin[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float bmax[3] = {FLT_MIN, FLT_MIN, FLT_MIN};
	for (ComponentBoundingBox boundingBox : scene->boundingBoxComponents) {
		AABB currentBB = boundingBox.GetWorldAABB();
		float3 currentBBMin = currentBB.minPoint;
		float3 currentBBMax = currentBB.maxPoint;
		bmin[0] = currentBBMin.x < bmin[0] ? currentBBMin.x : bmin[0];
		bmin[1] = currentBBMin.y < bmin[1] ? currentBBMin.y : bmin[1];
		bmin[2] = currentBBMin.z < bmin[2] ? currentBBMin.z : bmin[2];
		bmax[0] = currentBBMax.x > bmax[0] ? currentBBMax.x : bmax[0];
		bmax[1] = currentBBMax.y > bmax[1] ? currentBBMax.y : bmax[1];
		bmax[2] = currentBBMax.z > bmax[2] ? currentBBMax.z : bmax[2];
	}

	//
	// Step 1. Initialize build config.
	//

	// Init build configuration from GUI
	rcConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.cs = cellSize;
	cfg.ch = cellHeight;
	cfg.walkableSlopeAngle = agentMaxSlope;
	cfg.walkableHeight = static_cast<int>(ceilf(agentHeight / cfg.ch));
	cfg.walkableClimb = static_cast<int>(floorf(agentMaxClimb / cfg.ch));
	cfg.walkableRadius = static_cast<int>(ceilf(agentRadius / cfg.cs));
	cfg.maxEdgeLen = static_cast<int>(edgeMaxLen / cellSize);
	cfg.maxSimplificationError = edgeMaxError;
	cfg.minRegionArea = static_cast<int>(rcSqr(regionMinSize));		// Note: area = size*size
	cfg.mergeRegionArea = static_cast<int>(rcSqr(regionMergeSize)); // Note: area = size*size
	cfg.maxVertsPerPoly = static_cast<int>(vertsPerPoly);
	cfg.tileSize = (int) tileSize;
	cfg.borderSize = cfg.walkableRadius + 3; // Reserve enough padding.
	cfg.width = cfg.tileSize + cfg.borderSize * 2;
	cfg.height = cfg.tileSize + cfg.borderSize * 2;
	cfg.detailSampleDist = detailSampleDist < 0.9f ? 0 : cellSize * detailSampleDist;
	cfg.detailSampleMaxError = cellHeight * detailSampleMaxError;

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(cfg.bmin, bmin);
	rcVcopy(cfg.bmax, bmax);
	int gw = 0, gh = 0;
	rcCalcGridSize(bmin, bmax, cellSize, &gw, &gh);
	const int ts = (int) tileSize;
	const int tw = (gw + ts - 1) / ts;
	const int th = (gh + ts - 1) / ts;

	LOG("Building navigation:");
	LOG(" - %d x %d cells", cfg.width, cfg.height);
	LOG(" - %.1fK verts, %.1fK tris", verts.size() / 1000.0f, ntris / 1000.0f);

	int tileBits = rcMin((int) dtIlog2(dtNextPow2(tw * th * EXPECTED_LAYERS_PER_TILE)), 14);
	if (tileBits > 14) tileBits = 14;
	int polyBits = 22 - tileBits;
	maxTiles = 1 << tileBits;
	maxPolysPerTile = 1 << polyBits;

	// Tile cache params.
	dtTileCacheParams tcparams;
	memset(&tcparams, 0, sizeof(tcparams));
	rcVcopy(tcparams.orig, bmin);
	tcparams.cs = cellSize;
	tcparams.ch = cellHeight;
	tcparams.width = (int) tileSize;
	tcparams.height = (int) tileSize;
	tcparams.walkableHeight = agentHeight;
	tcparams.walkableRadius = agentRadius;
	tcparams.walkableClimb = agentMaxClimb;
	tcparams.maxSimplificationError = edgeMaxError;
	tcparams.maxTiles = tw * th * EXPECTED_LAYERS_PER_TILE;
	tcparams.maxObstacles = 128;

	dtFreeTileCache(tileCache);

	tileCache = dtAllocTileCache();
	if (!tileCache) {
		LOG("buildTiledNavigation: Could not allocate tile cache.");
		return false;
	}
	dtStatus status = tileCache->init(&tcparams, talloc, tcomp, tmproc);
	if (dtStatusFailed(status)) {
		LOG("buildTiledNavigation: Could not init tile cache.");
		return false;
	}

	dtFreeNavMesh(navMesh);

	navMesh = dtAllocNavMesh();
	if (!navMesh) {
		LOG("buildTiledNavigation: Could not allocate navmesh.");
		return false;
	}

	dtNavMeshParams params;
	memset(&params, 0, sizeof(params));
	rcVcopy(params.orig, bmin);
	params.tileWidth = tileSize * cellSize;
	params.tileHeight = tileSize * cellSize;
	params.maxTiles = maxTiles;
	params.maxPolys = maxPolysPerTile;

	status = navMesh->init(&params);
	if (dtStatusFailed(status)) {
		LOG("buildTiledNavigation: Could not init navmesh.");
		return false;
	}

	status = navQuery->init(navMesh, 2048);
	if (dtStatusFailed(status)) {
		LOG("buildTiledNavigation: Could not init Detour navmesh query");
		return false;
	}

	rcChunkyTriMesh* chunkyMesh = new rcChunkyTriMesh;
	if (!chunkyMesh) {
		LOG("buildTiledNavigation: Out of memory 'm_chunkyMesh'.");
		return false;
	}
	if (!rcCreateChunkyTriMesh(&verts[0], &tris[0], ntris, 256, chunkyMesh)) {
		LOG("buildTiledNavigation: Failed to build chunky mesh.");
		return false;
	}

	for (int y = 0; y < th; ++y) {
		for (int x = 0; x < tw; ++x) {
			TileCacheData tiles[MAX_LAYERS];
			memset(tiles, 0, sizeof(tiles));
			int ntiles = RasterizeTileLayers(&verts[0], verts.size(), ntris, ctx, chunkyMesh, x, y, cfg, tiles, MAX_LAYERS);

			for (int i = 0; i < ntiles; ++i) {
				TileCacheData* tile = &tiles[i];
				status = tileCache->addTile(tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0);
				if (dtStatusFailed(status)) {
					dtFree(tile->data);
					tile->data = 0;
					continue;
				}
			}
		}
	}

	// Build initial meshes
	for (int y = 0; y < th; ++y) {
		for (int x = 0; x < tw; ++x) {
			tileCache->buildNavMeshTilesAt(x, y, navMesh);
		}
	}
	ctx->stopTimer(RC_TIMER_TOTAL);

	const dtNavMesh* nav = navMesh;
	int navmeshMemUsage = 0;
	for (int i = 0; i < nav->getMaxTiles(); ++i) {
		const dtMeshTile* tile = nav->getTile(i);
		if (tile->header) {
			navmeshMemUsage += tile->dataSize;
		}
	}

	RELEASE(chunkyMesh);

	LOG("navmeshMemUsage = %.1f kB", navmeshMemUsage / 1024.0f);

	InitCrowd();

	return true;
}

void NavMesh::DrawGizmos(Scene* scene) {
	DebugDrawGL dds;

	glUseProgram(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glLoadMatrixf(App->camera->GetProjectionMatrix().Transposed().ptr());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glLoadMatrixf(App->camera->GetViewMatrix().Transposed().ptr());

	const float texScale = 1.0f / (cellSize * 10.0f);

	// Draw bounds
	float bmin[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float bmax[3] = {FLT_MIN, FLT_MIN, FLT_MIN};
	for (ComponentBoundingBox boundingBox : scene->boundingBoxComponents) {
		AABB currentBB = boundingBox.GetWorldAABB();
		float3 currentBBMin = currentBB.minPoint;
		float3 currentBBMax = currentBB.maxPoint;
		bmin[0] = currentBBMin.x < bmin[0] ? currentBBMin.x : bmin[0];
		bmin[1] = currentBBMin.y < bmin[1] ? currentBBMin.y : bmin[1];
		bmin[2] = currentBBMin.z < bmin[2] ? currentBBMin.z : bmin[2];
		bmax[0] = currentBBMax.x > bmax[0] ? currentBBMax.x : bmax[0];
		bmax[1] = currentBBMax.y > bmax[1] ? currentBBMax.y : bmax[1];
		bmax[2] = currentBBMax.z > bmax[2] ? currentBBMax.z : bmax[2];
	}

	DebugDrawGL dd;

	if (tileCache && drawMode == DRAWMODE_CACHE_BOUNDS) {
		DrawTiles(&dd, tileCache);
	}

	if (tileCache) {
		DrawObstacles(&dd, tileCache);
	}
	
	// Draw bounds
	duDebugDrawBoxWire(&dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], duRGBA(255, 255, 255, 128), 1.0f);

	// Tiling grid.
	int gw = 0, gh = 0;
	rcCalcGridSize(bmin, bmax, cellSize, &gw, &gh);
	const int tw = (gw + (int) tileSize - 1) / (int) tileSize;
	const int th = (gh + (int) tileSize - 1) / (int) tileSize;
	const float s = tileSize * cellSize;
	duDebugDrawGridXZ(&dd, bmin[0], bmin[1], bmin[2], tw, th, s, duRGBA(0, 0, 0, 64), 1.0f);

	if (navMesh && navQuery && (drawMode == DRAWMODE_NAVMESH || drawMode == DRAWMODE_NAVMESH_TRANS || drawMode == DRAWMODE_NAVMESH_BVTREE || drawMode == DRAWMODE_NAVMESH_NODES || drawMode == DRAWMODE_NAVMESH_PORTALS || drawMode == DRAWMODE_NAVMESH_INVIS)) {
		if (drawMode != DRAWMODE_NAVMESH_INVIS) {
			duDebugDrawNavMeshWithClosedList(&dd, *navMesh, *navQuery, navMeshDrawFlags /*|DU_DRAWNAVMESH_COLOR_TILES*/);
		}
		if (drawMode == DRAWMODE_NAVMESH_BVTREE) {
			duDebugDrawNavMeshBVTree(&dd, *navMesh);
		}
		if (drawMode == DRAWMODE_NAVMESH_PORTALS) {
			duDebugDrawNavMeshPortals(&dd, *navMesh);
		}
		if (drawMode == DRAWMODE_NAVMESH_NODES) {
			duDebugDrawNavMeshNodes(&dd, *navQuery);
		}
			
		duDebugDrawNavMeshPolysWithFlags(&dd, *navMesh, SAMPLE_POLYFLAGS_DISABLED, duRGBA(0, 0, 0, 128));
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static const int TILECACHESET_MAGIC = 'T' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'TSET';
static const int TILECACHESET_VERSION = 1;

struct TileCacheSetHeader {
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams meshParams;
	dtTileCacheParams cacheParams;
};

struct TileCacheTileHeader {
	dtCompressedTileRef tileRef;
	int dataSize;
};

void NavMesh::Load(Buffer<char>& buffer) {
	CleanUp();

	talloc = new LinearAllocator(32000);
	tcomp = new FastLZCompressor;
	tmproc = new MeshProcess;

	char* cursor = buffer.Data();

	// Read header.
	TileCacheSetHeader header;
	header = *((TileCacheSetHeader*) cursor);
	cursor += sizeof(TileCacheSetHeader);

	navMesh = dtAllocNavMesh();
	if (!navMesh) {
		LOG("Could not create Detour navmesh");
		return;
	}
	dtStatus status = navMesh->init(&header.meshParams);
	if (dtStatusFailed(status)) {
		LOG("Could not init Detour navmesh");
		return;
	}

	tileCache = dtAllocTileCache();
	if (!tileCache) {
		LOG("Could not allocate tileCache");
		return;
	}
	status = tileCache->init(&header.cacheParams, talloc, tcomp, tmproc);
	if (dtStatusFailed(status)) {
		LOG("Could not init tileCache");
		return;
	}

	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i) {
		TileCacheTileHeader tileHeader = *((TileCacheTileHeader*) cursor);
		cursor += sizeof(TileCacheTileHeader);

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*) malloc(tileHeader.dataSize);
		if (!data) break;
		memset(data, 0, tileHeader.dataSize);

		unsigned int _dataSize = sizeof(unsigned char) * tileHeader.dataSize;
		memcpy_s(data, _dataSize, cursor, _dataSize);		// IMPORTANT! DO NOT FREE DATA NOW BECAUSE IT WILL BE RELEASED WHEN CLOSING THE APPLICATION.
		cursor += _dataSize;

		dtCompressedTileRef tile = 0;
		tileCache->addTile(data, tileHeader.dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tile);

		if (tile) tileCache->buildNavMeshTile(tile, navMesh);
	}

	status = navQuery->init(navMesh, 2048);
	if (dtStatusFailed(status)) {
		LOG("Could not init Detour navmesh query");
		return;
	}

	InitCrowd();
}

void NavMesh::CleanUp() {
	dtFreeNavMesh(navMesh);
	navMesh = nullptr;
	
	dtFreeTileCache(tileCache);
	tileCache = nullptr;

	RELEASE(tmproc);
	RELEASE(tcomp);
	RELEASE(talloc);
}

Buffer<char> NavMesh::Save() {
	if (!tileCache || !navMesh) return Buffer<char>();

	int sizeData = 0;
	int numTiles = 0;
	for (int i = 0; i < tileCache->getTileCount(); ++i) {
		const dtCompressedTile* tile = tileCache->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;

		sizeData += tile->dataSize;
		sizeData += sizeof(TileCacheTileHeader);
		numTiles++;
	}

	// Store header.
	TileCacheSetHeader header;
	header.magic = TILECACHESET_MAGIC;
	header.version = TILECACHESET_VERSION;
	header.numTiles = numTiles;

	memcpy(&header.cacheParams, tileCache->getParams(), sizeof(dtTileCacheParams));
	memcpy(&header.meshParams, navMesh->getParams(), sizeof(dtNavMeshParams));

	int size = sizeof(TileCacheSetHeader) + sizeData;
	Buffer<char> buffer = Buffer<char>(size);
	char* cursor = buffer.Data();

	*((TileCacheSetHeader*) cursor) = header;
	cursor += sizeof(TileCacheSetHeader);

	// Store tiles.
	for (int i = 0; i < tileCache->getTileCount(); ++i) {
		const dtCompressedTile* tile = tileCache->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;

		TileCacheTileHeader tileHeader;
		tileHeader.tileRef = tileCache->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;

		*((TileCacheTileHeader*) cursor) = tileHeader;
		cursor += sizeof(TileCacheTileHeader);

		unsigned int _dataSize = sizeof(unsigned char) * tile->dataSize;
		memcpy_s(cursor, _dataSize, tile->data, _dataSize);
		cursor += _dataSize;

	}

	return buffer;
}

dtCrowd* NavMesh::GetCrowd() {
	return crowd;
}

bool NavMesh::IsGenerated() {
	return tileCache != nullptr;
}

dtNavMeshQuery* NavMesh::GetNavMeshQuery() {
	return navQuery;
}

dtNavMesh* NavMesh::GetNavMesh() {
	return navMesh;
}

dtTileCache* NavMesh::GetTileCache() {
	return tileCache;
}

void NavMesh::InitCrowd() {
	crowd->init(MAX_AGENTS, agentRadius, navMesh);

	// Make polygons with 'disabled' flag invalid.
	crowd->getEditableFilter(0)->setExcludeFlags(SAMPLE_POLYFLAGS_DISABLED);

	// Setup local avoidance params to different qualities.
	dtObstacleAvoidanceParams params;
	// Use mostly default settings, copy from dtCrowd.
	memcpy(&params, crowd->getObstacleAvoidanceParams(0), sizeof(dtObstacleAvoidanceParams));

	// Low (11)
	params.velBias = 0.5f;
	params.adaptiveDivs = 5;
	params.adaptiveRings = 2;
	params.adaptiveDepth = 1;
	crowd->setObstacleAvoidanceParams(0, &params);

	// Medium (22)
	params.velBias = 0.5f;
	params.adaptiveDivs = 5;
	params.adaptiveRings = 2;
	params.adaptiveDepth = 2;
	crowd->setObstacleAvoidanceParams(1, &params);

	// Good (45)
	params.velBias = 0.5f;
	params.adaptiveDivs = 7;
	params.adaptiveRings = 2;
	params.adaptiveDepth = 3;
	crowd->setObstacleAvoidanceParams(2, &params);

	// High (66)
	params.velBias = 0.5f;
	params.adaptiveDivs = 7;
	params.adaptiveRings = 3;
	params.adaptiveDepth = 3;

	crowd->setObstacleAvoidanceParams(3, &params);
}
