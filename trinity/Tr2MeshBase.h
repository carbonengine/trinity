////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2MeshBase_h
#define Tr2MeshBase_h

#include "Tr2MeshArea.h"
#include "ITr2Renderable.h"
#include "TriRenderBatch.h"
#include "Tr2DebugRenderer.h"
#include "Utilities/Tr2MaterialBoundsAdjustment.h"


BLUE_DECLARE( TriGeometryRes );

struct TriGeometryResSkeletonData;
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class TriRenderBatch;

// --------------------------------------------------------------------------------------
// Description:
//   A callback object for Tr2Mesh::GetBatches function. Callers of Tr2Mesh::GetBatches
//   can perform custom per-area operations on a render batch created by Tr2Mesh or even 
//   reject a batch from submitting.
// See Also:
//   Tr2Mesh
// --------------------------------------------------------------------------------------
struct ITr2MeshBatchCallback
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Per-area/batch callback function.
	// Arguments:
	//   area - Mesh area for which the render batch was created
	//   batch - A new render batch that is about to be submitted
	// Return Value:
	//   true If the batch needs to be submitted
	//   false If the batch needs to be dropped
	// ----------------------------------------------------------------------------------
	virtual bool ProcessBatch( Tr2MeshArea* area, TriRenderBatch* batch ) = 0;
};

BLUE_CLASS( Tr2MeshBase ):
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2MeshBase( IRoot* lockobj = NULL );
	~Tr2MeshBase();

	virtual void GetBatches( ITriRenderBatchAccumulator* batches,
		const Tr2MeshAreaVector* areas, 
		const Tr2PerObjectData* data,
		float screenSize = std::numeric_limits<float>::max(),
		ITr2MeshBatchCallback* callback = nullptr ) const;

	Tr2MeshAreaVector* GetAreas( TriBatchType areaType );
	const Tr2MeshAreaVector* GetAreas( TriBatchType areaType ) const;
	void CollectAreaBlocks( std::vector<TriRenderBatchAreaBlock>& areaBlockVector, TriBatchType areaType ) const;
	void CollectAreaBlocksWithSharedMaterial( TriRenderBatchAreaBlocksWithSharedMaterial& collector, TriBatchType areaType ) const;

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value );

	int GetMeshIndex() const { return m_meshIndex; };

	virtual CcpMath::AxisAlignedBox GetBounds( const Matrix* boneTransforms = nullptr ) const;
	virtual CcpMath::AxisAlignedBox GetAreaBounds( unsigned int areaIx, const Matrix* boneTransforms = nullptr ) const;

	bool GetBoundingBox( Vector3 & min, Vector3 & max ) const;

	bool BindToRig( const std::string* boneList, const int numBones, TriGeometryResSkeletonData* renderRig, bool forceRebind = false );

	virtual bool IsLoading() const = 0;

	const char* GetName() const { return m_name.c_str(); }
	const wchar_t* GetGeometryResPath() const;

	bool GetDisplay() const;

	virtual TriGeometryRes* GetGeometryResource() const = 0;

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	/////////////////////////////////////////////////////////////////////////////////////
	void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		);

	virtual void GetDebugOptions( Tr2DebugRendererOptions & options );
	virtual void RenderDebugInfo( const Matrix& worldTransform, ITr2DebugRenderer2& renderer );

	std::vector<Tr2MeshAreaPtr> GetAllAreas() const;

	Tr2MaterialBoundsAdjustment GetMaterialBoundsAdjustment() const;
	void SetMaterialBoundsAdjustment( const Tr2MaterialBoundsAdjustment& adjustment );

	void UseWithScreenSize( float screenSize, float worldRadius ) const;

protected:
	unsigned int FindJoint( const std::string* boneList, const int numBones, const char* name ) const;
	void CacheBounds();

protected:
	std::string m_name;
	bool m_display;
	int32_t	m_meshIndex;

	PTr2MeshAreaVector m_opaqueAreas;
	PTr2MeshAreaVector m_decalAreas;
	PTr2MeshAreaVector m_depthAreas;
	PTr2MeshAreaVector m_transparentAreas;
	PTr2MeshAreaVector m_additiveAreas;
	PTr2MeshAreaVector m_pickableAreas;
	PTr2MeshAreaVector m_mirrorAreas;
	PTr2MeshAreaVector m_decalNormalAreas;
	PTr2MeshAreaVector m_depthNormalAreas;
	PTr2MeshAreaVector m_opaquePrepassAreas;
	PTr2MeshAreaVector m_decalPrepassAreas;
	PTr2MeshAreaVector m_geometryEraserAreas;
	PTr2MeshAreaVector m_flareAreas;
	PTr2MeshAreaVector m_distortionAreas;

	PTr2MeshAreaVector* m_areaLookupArray[ TRIBATCHTYPE_COUNT_OF_BATCH_TYPES ];

	// skeleton/bone data
	std::vector<unsigned int> m_jointMappingAnimRig;
	const std::string *m_pBoneList;
	int m_numBones;
	TriGeometryResSkeletonData* m_renderRig;

	bool	m_forcedRebind;

	CcpMath::AxisAlignedBox m_cachedBounds;
	Tr2MaterialBoundsAdjustment m_boundsAdjustment;
};

TYPEDEF_BLUECLASS( Tr2MeshBase );
BLUE_DECLARE_VECTOR( Tr2MeshBase );


#endif // Tr2MeshBase_h