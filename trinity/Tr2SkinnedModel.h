#pragma once
#ifndef Tr2SkinnedModel_H
#define Tr2SkinnedModel_H

#include "Tr2Model.h"

struct TriGeometryResSkeletonData;
class Tr2PerObjectDataSkinned;

BLUE_DECLARE( Tr2SkinnedModel );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE_VECTOR( Tr2MeshArea );

namespace MR
{
	class Rig;
}

class Tr2SkinnedModel:
	public Tr2Model,
	public IInitialize,
	public INotify,
	public IListNotify,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	using Tr2Model::Lock;
	using Tr2Model::Unlock;

	Tr2SkinnedModel( IRoot* lockobj = NULL );
	~Tr2SkinnedModel();

	TriGeometryResSkeletonData* GetSkeleton() const;

	virtual void GetBatches( ITriRenderBatchAccumulator* batches,
							 TriBatchType batchType,
							 const Matrix& m, 
							 const Tr2PerObjectData* data );
	
	virtual bool HasTransparency() const;
	virtual bool GetBoundingBox( Vector3& min, Vector3& max );

	void ResetBindings();
	void BindToRig(  const std::string* boneList, const int numBones, bool forceRebind );

	//////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes* p );
	virtual void RebuildCachedData( BlueAsyncRes* p );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified(
		long event,		// BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		);

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	// set
	void SetGeometryResPath( const char* path ) { m_geometryResPath = path; }
	const char* GetGeometryResPath() { return m_geometryResPath.c_str(); }
	void SetSkeletonName( const char* name ) { m_skeletonName = name; }

	bool GetDynamicBoundingBox( const Matrix* boneTransforms, Vector3& minBounds, Vector3& maxBounds ) const;

protected:
    const std::string *m_pBoneList;
    int m_numBones;
	//MR::Rig* m_boundRig;
	bool m_areAllMeshesBound;

	std::string m_geometryResPath;
	TriGeometryResPtr m_geometryRes;
	std::string m_skeletonName;
	unsigned int m_skeletonIx;

	// This is a scaling factor that is applied only to the skin.
	// This is a workaround for an issue with rigs being built to a different
	// scale from the meshes - will hopefully be fixed before too long.
	Vector3 m_skinScale;

private:
	// helper for get batches
	void GetBatchesForArea( Tr2MeshAreaVector* areas, Tr2Mesh* mesh, ITriRenderBatchAccumulator* batches, const Matrix* pm, Tr2PerObjectDataSkinned* skinnedData );
};

TYPEDEF_BLUECLASS( Tr2SkinnedModel );

#endif