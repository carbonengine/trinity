#pragma once
#ifndef Tr2Mesh_H
#define Tr2Mesh_H

#include "Tr2MeshArea.h"
#include "Tr2HideableMixin.h"
#include "ITr2Renderable.h"
#include "TriRenderBatch.h"

#include "blue/Include/IUnloadable.h"

BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2VariableStore );
struct TriGeometryResSkeletonData;
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class TriRenderBatch;

namespace MR
{
	class Rig;
}

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

BLUE_CLASS( Tr2Mesh ):
	public IInitialize,
	public INotify,
	public IListNotify,
	public IBlueAsyncResNotifyTarget,
	public Tr2HideableMixin,
	public IUnloadable
{
public:
	EXPOSE_TO_BLUE();

	Tr2Mesh( IRoot* lockobj = NULL );
	~Tr2Mesh();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	virtual void GetBatches( ITriRenderBatchAccumulator* batches,
					 const Tr2MeshAreaVector* areas, 
					 const Tr2PerObjectData* data,
					 ITr2MeshBatchCallback* callback = nullptr ) const;

	Tr2MeshAreaVector* GetAreas( TriBatchType areaType );
	const Tr2MeshAreaVector* GetAreas( TriBatchType areaType ) const;
	void CollectAreaBlocks( std::vector<TriRenderBatchAreaBlock>& areaBlockVector, TriBatchType areaType ) const;
	
	void SetLowDetail( bool b ) { m_isLowDetail = b; }
	
	const char* GetMeshResPath() const { return m_meshResPath.c_str(); }
	void SetMeshResPath( const char* path );

	int GetMeshIndex() const { return m_meshIndex; };

	TriGeometryRes* GetGeometryResource() { return m_geometryResource; };
	void SetGeometryRes( TriGeometryRes* res );

	TriGeometryRes* GetLowDetailGeometryResource() { return m_lowDetailGeometryResource; }
	void SetLowDetailGeometryRes( TriGeometryRes* res );

	bool DeferGeometryLoad() const { return m_deferGeometryLoad; }
	void DeferGeometryLoad(bool val) { m_deferGeometryLoad = val; }

	virtual float CalcMeshSortValue( const Matrix& worldTransform );

    virtual bool GetBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool GetAreaBoundingBox( unsigned int areaIx, Vector3& min, Vector3& max ) const;
	bool GetAreaBasis( unsigned int areaIx, Vector3& pointOnTriangle, Vector3& edge1, Vector3& edge2 ) const;
	virtual bool GetBoundingSphere( Vector4& sphere );
	bool BindToRig( const std::string* boneList, const int numBones, TriGeometryResSkeletonData* renderRig, bool forceRebind = false );

	virtual bool IsLoading() const { return m_isLoading; }

	const char* GetName() const { return m_name.c_str(); }

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

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

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes* p );
	virtual void RebuildCachedData( BlueAsyncRes* p );

	//////////////////////////////////////////////////////////////////////////
	// IUnloadable
	virtual void UnloadWhenUnreferenced();
	virtual void ReloadWhenReferenced();

	// will take a list of engine flags at some point.
	void BindLowLevelShaders( const std::vector<unsigned int>& engineFlags, 
		bool overrideDefaultSituation = false,
		Tr2VariableStore* variableStore = NULL );

	// Is this Tr2Mesh waiting to bind low-level shaders?
	bool HasPendingLowLevelShaderBind( void ) const;
	// Attempt to execute pending low-level shader binding
	void ExecutePendingLowLevelShaderBind( void );

	void AddGeometryPreparedCallback( const BlueScriptCallback& callback );

private:
	void InitializeGeometryResource();
	void InitializeLowDetailGeometryResource();
	unsigned int FindJoint( const std::string* boneList, const int numBones, const char* name ) const;

	static void StaticResourceLoadFinished( void* pContext );
	static void StaticResourcePrepFinished( void* pContext );
	void DoPrepCallbacks();

	void PySetGeometryRes( TriGeometryRes* geometryRes );
	int GetAreasCount() const;
protected:
	std::string m_name;
	std::string m_meshResPath;
	std::string m_lowDetailMeshResPath;
	bool m_deferGeometryLoad;
	bool m_immutable;
	bool m_computeAccess;
	TriGeometryResPtr m_geometryResource;
	TriGeometryResPtr m_lowDetailGeometryResource;
	int	m_meshIndex;
	std::string m_geomResourceEx;

	bool m_isLowDetail;
	bool m_isLoading;
	CcpAtomic<uint32_t> m_resourceLoadCbId;
	CcpAtomic<uint32_t> m_resourcePrepCbId;

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

	// Bounding information from the geometry resource. This is set on a callback
	// once the geometry resource finishes loading, until then it is marked as
	// invalid and the GetBoundingSphere and GetBoundingBox functions return false.
	bool m_areBoundsValid;
	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	Vector4 m_boundingSphere;

	// used by shader binder
	void BindAreaShaders( Tr2MeshAreaVector* areas, 
		const std::vector<unsigned int>& engineFlags, bool overrideDefaultSituation, Tr2VariableStore* variableStore );

	std::vector<unsigned int> m_pendingBindSituationFlags;
	bool m_isBindPending;
	bool m_pendingDefaultOverride;
	Tr2VariableStorePtr m_pendingVariableStore;

	std::vector<BlueScriptCallback> m_geometryPreparedCallbacks;
};

TYPEDEF_BLUECLASS( Tr2Mesh );
BLUE_DECLARE_VECTOR( Tr2Mesh );

//
// Helpers for mesh sorting
//
struct Tr2MeshItem
{
	Tr2Mesh* m_mesh;
	float m_distance;
	bool operator<( const Tr2MeshItem& other ) const
	{
		return m_distance > other.m_distance;
	}
};
typedef TrackableStdVector<Tr2MeshItem> Tr2MeshItemList;

#endif
