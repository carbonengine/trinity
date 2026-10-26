// Copyright © 2000 CCP ehf.

#ifndef _TriGeometryRes_H_
#define _TriGeometryRes_H_

#include "Tr2CmfContent.h"
#include "Tr2GeometryProvider.h"
#include "../Utilities/BVH.h"

BLUE_DECLARE( TriGrannyRes );
class Tr2RenderContext;

enum class GrannyDeprecationLevel
{
	DO_NOTHING,
	LOG_ERROR,
	LOG_ERROR_AND_ASSERT
};

void HandleGrannyDeprecation( const std::wstring& path );

BLUE_CLASS( Tr2RaycastGeometryRes ) :
	public BlueAsyncRes
{
public:
	EXPOSE_TO_BLUE();

	void SetLodIndices( std::vector<int32_t> && lodIndices );
	BVH::BoundingVolumeHierarchy& GetBVH();

protected:
	LoadingResult DoLoad() override;
	bool DoPrepare() override;

private:
	std::vector<int32_t> m_lodIndices;
	BVH::BoundingVolumeHierarchy m_bvh;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2RaycastGeometryRes );


BLUE_CLASS_IMPL( TriGeometryRes )
class TriGeometryRes : public Tr2GeometryProvider
{
public:
	EXPOSE_TO_BLUE();

	TriGeometryRes( IRoot* lockobj = NULL );
	virtual ~TriGeometryRes();

	unsigned int GetAnimationCount() const;

#if WITH_GRANNY
	// query vertex component
	int GetVertexComponentOffset( const granny_mesh* myMesh, const char* componentName ) const;
#endif

	bool GetIntersectionPoints( const Vector3& pos, const Vector3& dir, RayCastResult& result, uint32_t areaIx = -1, float rayLength = std::numeric_limits<float>::infinity() ) override;

	void PrepareRayCaster() override;
	void ResetRayCaster() override;
	bool IsRayCasterReady() const override;
	bool HasRayCasterPreparationFailed() const override;
	void DestroyRayCaster() override;

	void PrepareFromGrannyRes( TriGrannyRes* g );

	//////////////////////////////////////////////////////////////////////////
	// IBlueResource
	//
	// Initialize is implemented by the BlueAsyncRes base class, but we
	// need to intercept it to reset data structures on reload
	void Initialize( const wchar_t* name, const wchar_t* ext ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
private:
	bool OnPrepareResources() override;

public:
	void ReleaseResources( TriStorage s ) override;

#if TRINITYDEV
	void GetDescription( std::string& desc ) override;
#endif

#if WITH_GRANNY
	granny_file_info* GetGrannyInfo() const;
#endif

	void Reload();

	TriGrannyResPtr m_sourceGranny;

	Be::Result<std::string> SaveMesh( const char* filename, uint32_t meshIndex ) const override;

	bool IsUsingCMF() const override;
	const cmf::Data* GetCMFData() const;

	void VisualizeBVH( Tr2DebugObjectReference owner, const Matrix& transform, ITr2DebugRenderer2& renderer ) const;

private:
#if WITH_GRANNY
	granny_file* m_pGrannyFile;
#endif

	Tr2CmfContents m_cmfContents;
	bool m_useCMF;

	struct
	{
		Tr2RaycastGeometryResPtr geometry = nullptr;
		int32_t sessions = 0;
	} m_bvh;

private:
	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in TriAsyncLoadedResource.
	LoadingResult DoLoad() override;
	bool DoPrepare() override;

#if WITH_GRANNY
	// Read granny file, keep data in m_pGrannyFile
	bool ReadGrannyFile();
	void ClearGrannyData();

	bool SetupMeshesFromGranny( granny_file_info* gi );
	void SetupSkeletonsFromGranny( granny_file_info* gi );
	void DetermineAreaBoundsAndVertCount( TriGeometryResAreaData& area, granny_mesh* grannyMesh, int bytesPerVertex );
	bool IsAreaSkinned( TriGeometryResAreaData& area, granny_mesh* grannyMesh, granny_file_info* gi, int bytesPerVertex );
	bool IsAreaMorphed( TriGeometryResAreaData& area, granny_mesh* myMesh, granny_file_info* gi );

	// Create D3D mesh from data in m_pGrannyFile
	bool CreateMeshesFromGrannyFile( granny_file_info* gi, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext );
	bool CreateLodFromGrannyMesh( granny_mesh* myMesh, TriGeometryResLodData* pMesh, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext, void* pVBOverride = NULL );

	// Extract audio geometry from lowest LOD for Wwise spatial audio
	void ExtractAudioGeometry( TriGeometryResMeshData* mesh, granny_mesh* grannyMesh );
#endif

	bool ReadCMFFile();
	void ClearCMFData();
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( TriGeometryRes );

#endif
