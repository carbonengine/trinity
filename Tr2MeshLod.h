////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2MeshLod_H
#define Tr2MeshLod_H

#include "Tr2MeshBase.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE_VECTOR( Tr2LodResource );

struct TriGeometryResSkeletonData;
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class TriRenderBatch;

namespace MR
{
	class Rig;
}

BLUE_CLASS( Tr2MeshLod ):
	public Tr2MeshBase
	// public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	Tr2MeshLod( IRoot* lockobj = NULL );
	~Tr2MeshLod();

	virtual bool IsLoading() const { return false; }

	void SelectLod( Tr2Lod lod );
	void AddAssociatedResource( Tr2LodResource* lr );
	void RemoveAssociatedResource( Tr2LodResource* lr );
	void ClearAssociatedResources();

	virtual TriGeometryRes* GetGeometryResource() const;
	void SetGeometryResource( Tr2LodResource* lodResource );


	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	///////////////////////////////////////////////////////////////////////////////////////
	//// IBlueAsyncResNotifyTarget
	//virtual void ReleaseCachedData( BlueAsyncRes* p );
	//virtual void RebuildCachedData( BlueAsyncRes* p );

private:
	unsigned int FindJoint( const std::string* boneList, const int numBones, const char* name ) const;

	static void StaticResourceLoadFinished( void* pContext );
	static void StaticResourcePrepFinished( void* pContext );

protected:
	Tr2LodResourcePtr m_geometryRes;

	bool m_immutable;
	bool m_computeAccess;

	bool m_isLoading;
	CcpAtomic<uint32_t> m_resourceLoadCbId;
	CcpAtomic<uint32_t> m_resourcePrepCbId;

	PTr2LodResourceVector m_associatedResources;
	Tr2Lod m_selectedLod;
};

TYPEDEF_BLUECLASS( Tr2MeshLod );
BLUE_DECLARE_VECTOR( Tr2MeshLod );

#endif
