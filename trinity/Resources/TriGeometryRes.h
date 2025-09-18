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

#include "Tr2SuballocatedBuffer.h"

constexpr uint32_t SHARED_BUFFER_BLOCK_SIZE = 32u * 1024u * 1024u;
constexpr uint32_t SHARED_BUFFER_MAX_SIZE = 2048u * 1024u * 1024u;
extern Tr2SuballocatedBuffer g_sharedBuffer;





BLUE_DECLARE( Tr2GpuStructuredBuffer );
BLUE_DECLARE( TriGrannyRes );
class Tr2RenderContext;

struct TriRtGeometryConstants
{
	uint32_t indexBufferId;
	uint32_t indexBufferStride;

	uint32_t indexOffset;

	uint32_t vertexBufferId;
	uint32_t vertexBufferStride;

	uint32_t positionOffset;
	uint32_t positionType;

	uint32_t normalOffset;
	uint32_t normalType;

	uint32_t tangentOffset;
	uint32_t tangentType;

	uint32_t bitangentOffset;
	uint32_t bitangentType;

	uint32_t texCoord0Offset;
	uint32_t texCoord0Type;

	uint32_t texCoord1Offset;
	uint32_t texCoord1Type;

	uint32_t texCoord2Offset;
	uint32_t texCoord2Type;

	uint32_t padding;
};

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

	Tr2RtBottomLevelAccelerationStructureAL m_staticBlas;
	bool m_isSkinned;
	Tr2ConstantBufferAL m_rtGeometryConstants;
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


struct TriJointBinding
{
	std::string m_name;
	Vector3 m_obbMin;
	Vector3 m_obbMax;
};

struct MeshDecalLodData
{
	uint32_t startIndex = 0;
	uint32_t primitiveCount = 0;
};

struct MeshDecalData
{
	Matrix invDecalMatrix = IdentityMatrix(); // used as a key
	Tr2SuballocatedBuffer::Allocation ib;
	std::vector<MeshDecalLodData> lods;
};

struct TriGeometryResMeshData
{
	TriGeometryResMeshData();

	std::string m_name;
	TrackableStdVector<TriGeometryResAreaData> m_areas;
	unsigned int m_vertexDeclaration;
	unsigned int m_bytesPerVertex;
	unsigned int m_vertexCount;
	unsigned int m_primitiveCount;

	bool m_allocationsValid;
	Tr2SuballocatedBuffer::Allocation m_vertexAllocation;
	Tr2SuballocatedBuffer::Allocation m_indexAllocation;

	std::vector<std::shared_ptr<MeshDecalData>> m_decals;

	// Index buffer with indexes in reversed order (used by hair/clothing)
	bool m_reversedIndicesValid;
	Tr2SuballocatedBuffer::Allocation m_reversedIndexAllocation;

	//Tr2SuballocatedBuffer::Allocation m_morphTargetAllocation;
	Tr2GpuStructuredBufferPtr m_morphTargetBuffer;

	int m_morphTargetAllocationCount;

	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	Vector4 m_boundingSphere;
	int32_t m_grannyMeshIndex;
	bool m_isLodMesh;
	TrackableStdVector<TriJointBinding> m_jointBindings;
	TriGeometryResVertexData* m_pVertexData;
	std::vector<float> m_uvDensities;

	struct LodRef
	{
		size_t meshIndex;
		float maxScreenSize;
	};
	std::vector<LodRef> m_lods;
};

uint32_t GetPrimitiveCount( const TriGeometryResMeshData& mesh, uint32_t index, uint32_t count );


struct TriGeometryResJointData
{
	std::string m_name;
	unsigned int m_parentJoint;
	Matrix m_inverseWorldTransform;
};

struct TriGeometryResSkeletonData
{
	TriGeometryResSkeletonData();

	unsigned int FindJoint( const char* name ) const;

	std::string m_name;
	TrackableStdVector<TriGeometryResJointData> m_joints;
};

struct TriGeometryResModelData
{
	std::string m_name;
	float m_translation[3];
	float m_orientation[4];
};

BLUE_CLASS( TriGeometryRes ):
	public BlueAsyncRes,
	public ICacheable,
	public Tr2DeviceResource,
	public ITr2InstanceData
{
public:
	EXPOSE_TO_BLUE();

	TriGeometryRes(IRoot* lockobj = NULL);	
	virtual ~TriGeometryRes();

	void RecalculateBoundingBox();
	void RecalculateBoundingSphere();
	Be::Result<std::string> CalculateBoundingBoxFromTransform( unsigned int meshIx, const Matrix& transform, std::pair<Vector3, Vector3>& bounds );

	unsigned int GetMeshCount() const;
	Be::Result<std::string> GetMeshName( unsigned int ix, std::string& name ) const;
	Be::Result<std::string> GetMeshAreaCount( unsigned int ix, int& count ) const;
	Be::Result<std::string> GetMeshAreaName( unsigned int meshIx, unsigned int areaIx, std::string& name ) const;
	TriGeometryResMeshData* GetMeshData( unsigned int meshIx ) const;
	TriGeometryResMeshData* GetMeshData( unsigned int meshIx, float screenSize ) const;
	TriGeometryResMeshData* GetMeshDataLod( unsigned int meshIx, int lodIndex ) const;
	int GetLodIndexForScreenSize( unsigned int meshIx, float screenSize ) const;

	unsigned int GetSkeletonCount() const;
	TriGeometryResSkeletonData* GetSkeletonData( unsigned int skelIx ) const;

	unsigned int GetModelCount() const;
	Be::Result<std::string> GetModelName( unsigned int ix, std::string& name ) const;
	TriGeometryResModelData* GetModelData( unsigned int modelIx ) const;

	unsigned int GetAreaCount( unsigned int meshIx ) const;
	Be::Result<std::string> GetAreaBoundingBoxFromScript( unsigned int meshIx, unsigned int areaIx, std::pair<Vector3, Vector3>& bounds );
	TriGeometryResAreaData* GetAreaData( unsigned int meshIx, unsigned int areaIx ) const;

	unsigned int GetAnimationCount() const;

	// query vertex component
	int GetVertexComponentOffset( const granny_mesh* myMesh, const char* componentName ) const;

	// Render multiple consecutive areas, starting at 'areaIx'
	bool RenderAreas( unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext& renderContext, bool reversed = false );
	bool RenderAreas( float screenSize, unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext& renderContext, bool reversed = false );

	void RebuildCachedData();
	
	bool GetIntersectionPoints( 
		const Vector3* pos, 
		const Vector3* dir, 
		Vector3* hitpointNear, 
		Vector3* hitpointNearNormal, 
		Vector3* hitpointFar, 
		Vector3* hitpointFarNormal,
		int* boneIndexNear,
		int* boneIndexFar,
		unsigned int areaIx=-1 );

	bool GetIntersectionPointNormalBone( 
		const Vector3* pos, 
		const Vector3* dir, 
		Vector3* hitpoint, 
		Vector3* normal,
		int* boneIndex,
		unsigned int areaIx=-1 );

	std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>> GetIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir );
	Be::Result<std::string> GetAreaIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir, int areaIx, std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>>& result );

	bool GetBoundingBox( unsigned int meshIx, Vector3& min, Vector3& max ) const;
	Be::Result<std::string> GetBoundingBoxFromScript( unsigned int meshIx, std::pair<Vector3, Vector3>& bounds ) const;
	bool GetAreaBoundingBox( unsigned int meshIx, unsigned int areaIx, Vector3& min, Vector3& max ) const;
	bool GetBoundingSphere( unsigned int meshIx, Vector4& sphere ) const;
	Be::Result<std::string> GetBoundingSphereFromScript( unsigned int meshIx, std::pair<Vector3, float>& bounds ) const;

	void PrepareFromGrannyRes( TriGrannyRes* g );

	BlueStdResult GetMeshVertexElements( size_t meshIndex, std::vector<std::pair<uint32_t, uint32_t>>& elements ) const;

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
	bool IsInstanceDataReady() const override;
	InstanceData GetInstanceData( unsigned int bufferIndex, float screenSize ) const override;
	unsigned int GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const override;
	CcpMath::AxisAlignedBox GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2GpuBuffer
	Tr2BufferAL* GetGpuBuffer( unsigned index );

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

	granny_file_info* GetGrannyInfo() const;
	
	static bool SaveMeshToGrannyFile( TriGeometryResMeshData* pMesh, const char* filename );

	//	Iterator functions for processing mesh data
	typedef void( *PerTriangleCallback )( void* context, const Vector3& p1, const Vector3& p2, const Vector3& p3 );
	void ProcessMeshTriangles( int meshIx, PerTriangleCallback cb, void* cbContext );

	void RequestReversedIndexBuffers();

	void Reload();

	// name for logging/debugging
	std::string m_name;

	TriGrannyResPtr m_sourceGranny;

	Be::Result<std::string> SaveMesh( const char* filename, uint32_t meshIndex ) const;

private:
	unsigned int m_memoryUse;
	TrackableStdVector<std::unique_ptr<TriGeometryResMeshData>> m_meshes;
	TrackableStdVector<std::unique_ptr<TriGeometryResMeshData>> m_meshLods;
	TrackableStdVector<std::unique_ptr<TriGeometryResModelData>> m_models;
	TrackableStdVector<std::unique_ptr<TriGeometryResSkeletonData>> m_skeletons;

	granny_file* m_pGrannyFile;
	granny_file_info* m_inMemoryInfo;

	uint32_t m_forcedLodIndex = 0;
	bool m_forceLod = false;
	bool m_reversedIndexBuffersRequested = false;

private:
	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in TriAsyncLoadedResource.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();

	// Read granny file, keep data in m_pGrannyFile
	bool ReadGrannyFile( granny_file_info* gi = NULL );
	void ClearGrannyData();

	void SetupModels( granny_file_info* gi );
	bool SetupMeshes( granny_file_info* gi );
	void SetupSkeletons( granny_file_info* gi );
	void DetermineAreaBoundsAndVertCount( TriGeometryResAreaData& area, granny_mesh* myMesh, int bytesPerVertex );
	bool IsAreaSkinned( TriGeometryResAreaData& area, granny_mesh* myMesh, granny_file_info* gi, int bytesPerVertex );
	
	// Create D3D mesh from data in m_pGrannyFile
	bool CreateMeshesFromGrannyFile( granny_file_info * gi, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext & renderContext );
	bool CreateMeshFromGrannyMesh( granny_mesh* myMesh, TriGeometryResMeshData* pMesh, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext, void* pVBOverride = NULL );
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN(TriGeometryRes);

#endif
