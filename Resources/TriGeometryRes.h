/* 
	*************************************************************************************

	TriGeometryRes.h

	Created:   October 2006
	OS:        Win32
	Project:   Trinity

	Description:   


	Dependencies:

		Blue, DirectX 9.0

	(c) CCP 2000


	*************************************************************************************
*/

#ifndef _TriGeometryRes_H_
#define _TriGeometryRes_H_

#include "Tr2DeviceResource.h"

#include "include/ITr2InstanceData.h"
#include "include/ITr2GpuBuffer.h"

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( Tr2ShaderMaterial);
class Tr2RenderContext;

struct TriGeometryResAreaData
{
	TriGeometryResAreaData();

	std::string m_name;
	int m_firstIndex;
	int m_primitiveCount;
	int m_vertexCount;
	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	TrackableStdVector<int> m_jointBindings;
};

struct TriGeometryResVertexData
{
	// main buffer
	granny_uint8* m_pBuffer;
	// pointer to different vertex components used in skinning
	uint8_t* m_pBoneIndex0;
	uint8_t* m_pBoneIndex1;
	uint8_t* m_pBoneIndex2;
	uint8_t* m_pBoneIndex3;
	uint8_t* m_pBoneWeight0;
	uint8_t* m_pBoneWeight1;
	uint8_t* m_pBoneWeight2;
	uint8_t* m_pBoneWeight3;
	uint8_t* m_pSrcPosition;
	uint8_t* m_pSrcNormal;
	uint8_t* m_pSrcTangent;
	uint8_t* m_pSrcBinormal;
	unsigned int m_dstPositionOffset;
	unsigned int m_dstNormalOffset;
	unsigned int m_dstTangentOffset;
	unsigned int m_dstBinormalOffset;
};

// -------------------------------------------------------------
// Description:
//   Per-mesh data for intersection tests.
// -------------------------------------------------------------
struct TriGeometryCollisionData
{
	// Aligned array of vertex positions
	XMVECTOR* m_positions;
	// Array of texture UV coordinates
	Vector2* m_texCoords;
	// Array of vertex indexes
	unsigned int* m_indexes;
};

struct TriGeometryResMeshData
{
	TriGeometryResMeshData();
	~TriGeometryResMeshData();

	bool BuildCollisionData();

	std::string m_name;
	TrackableStdVector<TriGeometryResAreaData> m_areas;
	unsigned int m_vertexDeclaration;
	unsigned int m_bytesPerVertex;
	unsigned int m_vertexCount;
	unsigned int m_primitiveCount;
	Tr2VertexBufferAL	m_vertexBuffer;
	Tr2GpuBufferAL      m_shaderResourceBuffer;
	Tr2IndexBufferAL	m_indexBuffer;
	// Index buffer with indexes in reversed order (used by hair/clothing)
	Tr2IndexBufferAL	m_reversedIndexBuffer;
	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	Vector4 m_boundingSphere;
	bool m_hasPerMeshAreaBoneBindings;
	TrackableStdVector<std::string> m_jointBindings;
	TriGeometryResVertexData* m_pVertexData;
	TriGeometryCollisionData m_collisionData;
};

struct TriGeometryResJointData
{
	std::string m_name;
	unsigned int m_parentJoint;
	Matrix m_inverseWorldTransform;
};

struct TriGeometryResSkeletonData
{
	TriGeometryResSkeletonData();

	unsigned int FindJoint( const char* name );

	std::string m_name;
	TrackableStdVector<TriGeometryResJointData> m_joints;
};

struct TriGeometryResModelData
{
	std::string m_name;
	float m_translation[3];
	float m_orientation[4];
};

enum TriGeometryCollisionResultFlags
{
	COLLISION_RESULT_ANY,
	COLLISION_RESULT_CLOSEST,
};

enum TriGeometryCollisionCullingFlags
{
	COLLISION_CULL_CCW,
	COLLISION_CULL_CW,
	COLLISION_CULL_NONE,
};

BLUE_CLASS( TriGeometryRes ):
	public BlueAsyncRes,
	public ICacheable,
	public Tr2DeviceResource,
	public ITr2InstanceData,
	public ITr2GpuBuffer
{
public:
	EXPOSE_TO_BLUE();

	TriGeometryRes(IRoot* lockobj = NULL);	
	virtual ~TriGeometryRes();

	void RecalculateBoundingBox();
	void RecalculateBoundingSphere();
	Be::Result<std::string> CalculateBoundingBoxFromTransform( unsigned int meshIx, const Matrix& transform, std::pair<Vector3, Vector3>& bounds );

	void SetIsDynamic( bool isDynamic );

	unsigned int GetMeshCount();
	Be::Result<std::string> GetMeshName( unsigned int ix, std::string& name );
	Be::Result<std::string> GetMeshAreaCount( unsigned int ix, int& count );
	Be::Result<std::string> GetMeshAreaName( unsigned int meshIx, unsigned int areaIx, std::string& name );
	TriGeometryResMeshData* GetMeshData( unsigned int meshIx );

	unsigned int GetSkeletonCount() const;
	TriGeometryResSkeletonData* GetSkeletonData( unsigned int skelIx );

	unsigned int GetModelCount();
	Be::Result<std::string> GetModelName( unsigned int ix, std::string& name );
	TriGeometryResModelData* GetModelData( unsigned int modelIx );

	unsigned int GetAreaCount( unsigned int meshIx );
	Be::Result<std::string> GetAreaBoundingBoxFromScript( unsigned int meshIx, unsigned int areaIx, std::pair<Vector3, Vector3>& bounds );
	TriGeometryResAreaData* GetAreaData( unsigned int meshIx, unsigned int areaIx );

	unsigned int GetAnimationCount();

	// query vertex component
	int GetVertexComponentOffset( const granny_mesh* myMesh, const char* componentName ) const;

	// Render a single area
	bool RenderArea( unsigned int meshIx, unsigned int areaIx, Tr2RenderContext& renderContext, bool reversed = false );

	// Render multiple consecutive areas, starting at 'areaIx'
    bool RenderAreas( unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext& renderContext, bool reversed = false );

	// Render multiple consecutive areas, starting at 'areaIx' using the provided vertexbuffer
    bool RenderAreasFromDynamicVertexBuffer( Tr2VertexBufferAL& vertexBuffer, unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, bool reversed = false );
	
	// Render all areas in one draw call
	bool RenderAsOneArea( unsigned int meshIx );

	void RebuildCachedData();
	
	bool GetIntersectionPoints( 
		const Vector3* pos, 
		const Vector3* dir, 
		Vector3* hitpointNear, 
		Vector3* hitpointNearNormal, 
		Vector3* hitpointFar, 
		Vector3* hitpointFarNormal );
	
	bool GetIntersectionPointAndNormal( 
		const Vector3* pos, 
		const Vector3* dir, 
		Vector3* hitpoint, 
		Vector3* normal );

	std::pair<bool, std::pair<Vector3, Vector3>> GetIntersectionPointAndNormalFromScript( const Vector3& pos, const Vector3& dir );

	std::pair<float, Vector3> GetClosestVertex( const Vector3& pos );

	bool GetBoundingBox( unsigned int meshIx, Vector3& min, Vector3& max ) const;
	Be::Result<std::string> GetBoundingBoxFromScript( unsigned int meshIx, std::pair<Vector3, Vector3>& bounds ) const;
	bool GetAreaBoundingBox( unsigned int meshIx, unsigned int areaIx, Vector3& min, Vector3& max ) const;
	bool GetAreaBasis( unsigned int meshIx, unsigned int areaIx, Vector3& pointOnTriangle, Vector3& edge1, Vector3& edge2 ) const;
	bool GetBoundingSphere( unsigned int meshIx, Vector4& sphere );
	Be::Result<std::string> GetBoundingSphereFromScript( unsigned int meshIx, std::pair<Vector3, float>& bounds );

	void PrepareFromGrannyRes( TriGrannyRes* g );

	bool GetRayAreaIntersection( const Vector3& rayOrigin, 
								 const Vector3& rayDirection, 
								 unsigned int meshIx, 
								 unsigned int areaIx, 
								 TriGeometryCollisionResultFlags resultFlags, 
								 TriGeometryCollisionCullingFlags culling, 
								 Vector3& position, 
								 Vector2& uv );

	std::pair<bool, std::pair<Vector3, Vector2>> GetRayAreaIntersectionFromScript( const Vector3& rayOrigin, 
		const Vector3& rayDirection, 
		unsigned int meshIx, 
		unsigned int areaIx, 
		TriGeometryCollisionResultFlags resultFlags, 
		TriGeometryCollisionCullingFlags culling );


	//////////////////////////////////////////////////////////////////////////
	// IBlueResource
	//
	// Initialize is implemented by the BlueAsyncRes base class, but we
	// need to intercept it to reset data structures on reload
	void Initialize( const wchar_t* name, const wchar_t* ext );

	// Given a granny file that's already in memory, build the meshes etc directly from that data.
	// This is equivalent to DoLoad from resman.
	bool InitializeFromMemory( granny_file_info* gfi );
	// After calling InitializeFromMemory, call DoPrepareFromMemory from the main thread. The data
	// originally passed to InitializeFromMemory still needs to be alive.
	// If successful, the geometry res is marked as Good and Prepared.
	void DoPrepareFromMemory();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InstanceData
	bool IsInstanceDataReady() const;
	unsigned int GetInstanceBufferCount() const;
	unsigned int GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const;
	unsigned int GetInstanceBufferVertexCount( unsigned int bufferIndex ) const;
	void GetVertexBuffer( unsigned int bufferIndex, Tr2VertexBufferAL*& buffer, unsigned& stride );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2GpuBuffer
	Tr2GpuBufferAL* GetGpuBuffer( unsigned index );

	/////////////////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
private:
	bool OnPrepareResources();
public:				
	void ReleaseResources( TriStorage s );

	void ReleaseResourcesHelper();
#if TRINITYDEV
	virtual void GetDescription( std::string& desc );
#endif

	granny_file_info* GetGrannyInfo();
	
	static void PrepareGrannyFileGeometry( granny_file_info* gi );
	static bool SaveMeshToGrannyFile( TriGeometryResMeshData* pMesh, const char* filename );

	float GetMeshSurfaceArea( int meshIx );

    // Insert custom dynamically generated mesh. Ownership of inserted object 
    // is moved to this class. Returns index to inserted mesh within this container. 
    // TODO How should this be done? - tatu
    int PushMesh( TriGeometryResMeshData* mesh );

	//	Iterator functions for processing mesh data
	typedef void( *PerTriangleCallback )( void* context, const Vector3& p1, const Vector3& p2, const Vector3& p3 );
	void ProcessMeshTriangles( int meshIx, PerTriangleCallback cb, void* cbContext );

	//	Iterator functions for processing mesh data
	typedef void( *PerVertexCallback )( void* context, const Vector3& p1, const Vector3& n1 );
	void ProcessMeshVertices( int meshIx, PerVertexCallback cb, void* cbContext );

	void ReverseIndexBuffer( unsigned int meshIx, Tr2RenderContext& renderContext );

	void Reload();

	// name for logging/debugging
	std::string m_name;

	bool	m_immutable;
	bool	m_computeAccess;

private:
	void* m_data;
	uint32_t m_dataSize;
	unsigned int m_memoryUse;
	TrackableStdVector<TriGeometryResMeshData*> m_meshes;
	TrackableStdVector<TriGeometryResModelData*> m_models;
	TrackableStdVector<TriGeometryResSkeletonData*> m_skeletons;

	bool m_isDynamicGeometry;
	granny_file* m_pGrannyFile;
	granny_file_info* m_inMemoryInfo;

	typedef void (*PerVertexUVCallback)( void* context, const Vector3& p1, const Vector3& n1, const Vector2& uv1 );
	void ProcessMeshVerticesWithUV( int meshIx, PerVertexUVCallback cb, void* cbContext );

private:
	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in TriAsyncLoadedResource.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();
	virtual void OnCloseStream();

	// Read granny file, keep data in m_pGrannyFile
	bool ReadGrannyFile( granny_file_info* gi = NULL );
	void ClearGrannyData();

	void SetupModels( granny_file_info* gi );
	bool SetupMeshes( granny_file_info* gi );
	void SetupSkeletons( granny_file_info* gi );
	void DetermineAreaBoundsAndVertCount( TriGeometryResAreaData& area, granny_mesh* myMesh, int bytesPerVertex );
	void DetermineAreaBones( TriGeometryResAreaData& area, granny_mesh* myMesh, int bytesPerVertex );
	
	// Create D3D mesh from data in m_pGrannyFile
	bool CreateMeshesFromGrannyFile( granny_file_info* gi, Tr2PrimaryRenderContext& renderContext );
	bool CreateMeshFromGrannyMesh( granny_mesh* myMesh, TriGeometryResMeshData* pMesh, Tr2PrimaryRenderContext& renderContext, void* pVBOverride = NULL );
	bool CreateD3DVertexBuffer( TriGeometryResMeshData* pMesh, int vtxCount, int bytesPerVtx, const granny_mesh* mesh, const void* pSrc, const granny_data_type_definition* grnVtxDecl, bool fullFloat, Tr2PrimaryRenderContext& renderContext );
	bool CreateSystemVertexBuffer( TriGeometryResMeshData* pMesh, int vtxCount, int bytesPerVtx, const granny_mesh* mesh, const void* pSrc, const granny_data_type_definition* grnVtxDecl, bool fullFloat );
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN(TriGeometryRes);

#endif
