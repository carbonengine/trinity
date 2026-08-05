// Copyright © 2000 CCP ehf.

#ifndef _TriGeometryRes_H_
#define _TriGeometryRes_H_

#include "Tr2DeviceResource.h"

#include "include/ITr2InstanceData.h"
#include "include/ITr2GpuBuffer.h"

#include "Tr2CmfContent.h"
#include "Tr2SuballocatedBuffer.h"

constexpr uint32_t SHARED_BUFFER_BLOCK_SIZE = 32u * 1024u * 1024u;
constexpr uint32_t SHARED_BUFFER_MAX_SIZE = 2048u * 1024u * 1024u;
extern Tr2SuballocatedBuffer g_sharedBuffer;



BLUE_DECLARE( TriGrannyRes );
class Tr2RenderContext;


enum class GrannyDeprecationLevel
{
	DO_NOTHING,
	LOG_ERROR,
	LOG_ERROR_AND_ASSERT
};

void HandleGrannyDeprecation( const std::wstring& path );

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

struct TriMorphTargetGeometryConstants
{
	uint32_t vertexBufferStride;

	uint32_t positionOffset;
	uint32_t positionType;

	uint32_t tangentOffset;
	uint32_t tangentType;

	uint32_t vertexCount;
};

struct TriGeometryResAreaData
{
	TriGeometryResAreaData();

	std::string m_name;
	int m_firstIndex;
	int m_primitiveCount;
	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	TrackableStdVector<int> m_jointBindings;

	Tr2RtBottomLevelAccelerationStructureAL m_staticBlas;
	bool m_isSkinned;
	bool m_isMorphed;
	Tr2ConstantBufferAL m_rtGeometryConstants;
};


struct TriJointBinding
{
	std::string m_name;
	Vector3 m_obbMin;
	Vector3 m_obbMax;
};

struct MeshDecalLodData
{
	uint32_t m_startIndex = 0;
	uint32_t m_primitiveCount = 0;
};

struct MeshDecalData
{
	Matrix m_inverseDecalMatrix = IdentityMatrix(); // used as a key
	Tr2SuballocatedBuffer::Allocation m_indexBuffer;
	uint32_t m_lodMask; //A bit mask of which LODs have been loaded.
	std::vector<MeshDecalLodData> m_lods;
};

struct TriGeometryResMeshData;

extern bool g_eveIsAudioOcclusionGeometryEnabled;

struct AudioGeometryResData
{
	uint64_t m_id;

	std::vector<Vector3> m_vertices;
	std::vector<uint32_t> m_indices;

	Vector3 m_minBounds;
	Vector3 m_maxBounds;

	AudioGeometryResData() :
		m_id( s_nextId++ )
	{
	}

private:
	static std::atomic<uint64_t> s_nextId;
};

struct TriGeometryResLodData
{
	TriGeometryResLodData();

	TriGeometryResMeshData* m_mesh;

	int32_t m_grannyMeshIndex;

	std::string m_name;

	int32_t m_originalLodIndex;
	float m_maxScreenSize;

	unsigned int m_vertexCount;
	unsigned int m_primitiveCount;

	std::vector<float> m_uvDensities;

	TrackableStdVector<TriGeometryResAreaData> m_areas;

	bool m_allocationsValid;
	Tr2SuballocatedBuffer::Allocation m_vertexAllocation;
	Tr2SuballocatedBuffer::Allocation m_indexAllocation;

	Tr2SuballocatedBuffer::Allocation m_morphTargetAllocation;

	std::vector<std::string> m_morphTargetNames;
	std::vector<float> m_morphTargetDeformationAmounts;
	std::vector<bool> m_isBakedMorphTarget;

	unsigned int m_morphVertexDeclaration;
	unsigned int m_bytesPerMorphTargetVertex;

	// Index buffer with indexes in reversed order (used by hair/clothing)
	bool m_reversedIndicesValid;
	Tr2SuballocatedBuffer::Allocation m_reversedIndexAllocation;
};

struct TriGeometryResMeshData
{
	TriGeometryResMeshData();

	std::string m_name;

	unsigned int m_vertexDeclarationHandle;
	unsigned int m_bytesPerVertex;

	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	Vector4 m_boundingSphere;

	TrackableStdVector<TriJointBinding> m_jointBindings;

	std::unique_ptr<AudioGeometryResData> m_audioGeometry;
	std::vector<std::shared_ptr<MeshDecalData>> m_decals;

	uint32_t m_lodMask; //A bit mask of which LODs have been loaded.
	TrackableStdVector<std::unique_ptr<TriGeometryResLodData>> m_lods;
};

uint32_t GetPrimitiveCount( const TriGeometryResLodData& lod, uint32_t index, uint32_t count );

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

BLUE_CLASS( TriGeometryRes ) :
	public BlueAsyncRes,
	public ICacheable,
	public Tr2DeviceResource,
	public ITr2InstanceData
{
public:
	EXPOSE_TO_BLUE();

	TriGeometryRes( IRoot* lockobj = NULL );
	virtual ~TriGeometryRes();

	void RecalculateBoundingBox();
	void RecalculateBoundingSphere();
	Be::Result<std::string> CalculateBoundingBoxFromTransform( unsigned int meshIx, const Matrix& transform, std::pair<Vector3, Vector3>& bounds );

	unsigned int GetMeshCount() const;
	Be::Result<std::string> GetMeshName( unsigned int meshIx, std::string& name ) const;
	Be::Result<std::string> GetMeshAreaCount( unsigned int meshIx, int& count ) const;
	Be::Result<std::string> GetMeshAreaName( unsigned int meshIx, unsigned int areaIx, std::string& name ) const;
	TriGeometryResMeshData* GetMeshData( unsigned int meshIx ) const;
	TriGeometryResLodData* GetMeshLod( unsigned int meshIx, float screenSize ) const;
	TriGeometryResLodData* GetMeshLod( unsigned int meshIx, int lodIndex ) const;
	int GetLodIndexForScreenSize( unsigned int meshIx, float screenSize ) const;


	const AudioGeometryResData* GetAudioGeometry( unsigned int meshIx ) const;

	unsigned int GetSkeletonCount() const;
	TriGeometryResSkeletonData* GetSkeletonData( unsigned int skelIx ) const;

	unsigned int GetAreaCount( unsigned int meshIx ) const;
	Be::Result<std::string> GetAreaBoundingBoxFromScript( unsigned int meshIx, unsigned int areaIx, std::pair<Vector3, Vector3>& bounds );
	TriGeometryResAreaData* GetAreaData( unsigned int meshIx, unsigned int areaIx ) const;

	unsigned int GetAnimationCount() const;

#if WITH_GRANNY
	// query vertex component
	int GetVertexComponentOffset( const granny_mesh* myMesh, const char* componentName ) const;
#endif

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
		unsigned int areaIx = -1 );

	bool GetIntersectionPointNormalBone(
		const Vector3* pos,
		const Vector3* dir,
		Vector3* hitpoint,
		Vector3* normal,
		int* boneIndex,
		unsigned int areaIx = -1 );

	std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>> GetIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir );
	Be::Result<std::string> GetAreaIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir, int areaIx, std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>>& result );

	bool GetBoundingBox( unsigned int meshIx, Vector3& min, Vector3& max ) const;
	Be::Result<std::string> GetBoundingBoxFromScript( unsigned int meshIx, std::pair<Vector3, Vector3>& bounds ) const;
	bool GetAreaBoundingBox( unsigned int meshIx, unsigned int areaIx, Vector3& min, Vector3& max ) const;
	bool GetBoundingSphere( unsigned int meshIx, Vector4& sphere ) const;
	Be::Result<std::string> GetBoundingSphereFromScript( unsigned int meshIx, std::pair<Vector3, float>& bounds ) const;

	void PrepareFromGrannyRes( TriGrannyRes * g );

	BlueStdResult GetMeshVertexElements( size_t meshIndex, std::vector<std::pair<uint32_t, uint32_t>>& elements ) const;

	//////////////////////////////////////////////////////////////////////////
	// IBlueResource
	//
	// Initialize is implemented by the BlueAsyncRes base class, but we
	// need to intercept it to reset data structures on reload
	void Initialize( const wchar_t* name, const wchar_t* ext );

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
	virtual void GetDescription( std::string & desc );
#endif

#if WITH_GRANNY
	granny_file_info* GetGrannyInfo() const;
#endif

#if WITH_GRANNY
	static bool SaveMeshToGrannyFile( TriGeometryResMeshData & mesh, const char* filename );
#endif
	static bool SaveMeshToCMFFile( TriGeometryResMeshData & mesh, const char* filename );

	//	Iterator functions for processing mesh data
	typedef void ( *PerTriangleCallback )( void* context, const Vector3& p1, const Vector3& p2, const Vector3& p3 );
	void ProcessMeshTriangles( int meshIx, PerTriangleCallback cb, void* cbContext );

	void Reload();

	// name for logging/debugging
	std::string m_name;

	TriGrannyResPtr m_sourceGranny;

	Be::Result<std::string> SaveMesh( const char* filename, uint32_t meshIndex ) const;

	bool IsUsingCMF() const;
	const cmf::Data* GetCMFData() const;

private:
	unsigned int m_memoryUse;
	TrackableStdVector<std::unique_ptr<TriGeometryResMeshData>> m_meshes;
	TrackableStdVector<std::unique_ptr<TriGeometryResSkeletonData>> m_skeletons;

#if WITH_GRANNY
	granny_file* m_pGrannyFile;
#endif

	Tr2CmfContents m_cmfContents;
	bool m_useCMF;

	int32_t m_forcedLodIndex = -1;
	bool m_forceLod = false;

private:
	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in TriAsyncLoadedResource.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();

#if WITH_GRANNY
	// Read granny file, keep data in m_pGrannyFile
	bool ReadGrannyFile();
	void ClearGrannyData();

	bool SetupMeshes( granny_file_info * gi );
	void SetupSkeletons( granny_file_info * gi );
	void DetermineAreaBoundsAndVertCount( TriGeometryResAreaData & area, granny_mesh * grannyMesh, int bytesPerVertex );
	bool IsAreaSkinned( TriGeometryResAreaData & area, granny_mesh * grannyMesh, granny_file_info * gi, int bytesPerVertex );
	bool IsAreaMorphed( TriGeometryResAreaData & area, granny_mesh * myMesh, granny_file_info * gi );

	// Create D3D mesh from data in m_pGrannyFile
	bool CreateMeshesFromGrannyFile( granny_file_info * gi, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext & renderContext );
	bool CreateLodFromGrannyMesh( granny_mesh * myMesh, TriGeometryResLodData * pMesh, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext & renderContext, void* pVBOverride = NULL );

	// Extract audio geometry from lowest LOD for Wwise spatial audio
	void ExtractAudioGeometry( TriGeometryResMeshData * mesh, granny_mesh * grannyMesh );
#endif

	bool ReadCMFFile();
	void ClearCMFData();

	bool SetupMeshes( const cmf::Data& cmfData );
	void SetupSkeletons( const cmf::Data& cmfData );
	bool CreateMeshesFromCMFFile( Tr2CmfContents & cmfContents, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext & renderContext );
	bool CreateLodFromCMFMesh( Tr2CmfContents & cmfContents, const cmf::Mesh& cmfMesh, const cmf::MeshLod& cmfMeshLod, TriGeometryResLodData* pMesh, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext );
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( TriGeometryRes );

#endif
