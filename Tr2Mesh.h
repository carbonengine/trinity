#pragma once
#ifndef Tr2Mesh_H
#define Tr2Mesh_H

#include "Tr2MeshBase.h"
#include "Tr2MeshArea.h"
#include "Tr2HideableMixin.h"
#include "ITr2Renderable.h"
#include "TriRenderBatch.h"

#include "blue/Include/IUnloadable.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE_VECTOR( Tr2LodResource );

struct TriGeometryResSkeletonData;
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class TriRenderBatch;

namespace MR
{
	class Rig;
}

BLUE_CLASS( Tr2Mesh ):
	public Tr2MeshBase,
	public IInitialize,
	public INotify,
	public IBlueAsyncResNotifyTarget,
	public IUnloadable
{
public:
	EXPOSE_TO_BLUE();

	Tr2Mesh( IRoot* lockobj = NULL );
	~Tr2Mesh();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	void SetLowDetail( bool b ) { }
	
	const char* GetMeshResPath() const { return m_meshResPath.c_str(); }
	void SetMeshResPath( const char* path );

	virtual TriGeometryRes* GetGeometryResource() const;;
	void SetGeometryRes( TriGeometryRes* res );

	bool DeferGeometryLoad() const { return m_deferGeometryLoad; }
	void DeferGeometryLoad(bool val) { m_deferGeometryLoad = val; }

	virtual bool IsLoading() const { return m_isLoading; }

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

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

private:
	void InitializeGeometryResource();

	static void StaticResourceLoadFinished( void* pContext );
	static void StaticResourcePrepFinished( void* pContext );

	void PySetGeometryRes( TriGeometryRes* geometryRes );
	int GetAreasCount() const;

protected:
	std::string m_meshResPath;
	bool m_deferGeometryLoad;
	bool m_immutable;
	bool m_computeAccess;
	TriGeometryResPtr m_geometryResource;
	std::string m_geomResourceEx;

	bool m_isLoading;
	CcpAtomic<uint32_t> m_resourceLoadCbId;
	CcpAtomic<uint32_t> m_resourcePrepCbId;

	// used by shader binder
	void BindAreaShaders( Tr2MeshAreaVector* areas, 
		const std::vector<unsigned int>& engineFlags, bool overrideDefaultSituation, Tr2VariableStore* variableStore );

	std::vector<unsigned int> m_pendingBindSituationFlags;
	bool m_isBindPending;
	bool m_pendingDefaultOverride;
	Tr2VariableStorePtr m_pendingVariableStore;

	PTr2LodResourceVector m_lodResources;
	Tr2Lod m_selectedLod;
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
