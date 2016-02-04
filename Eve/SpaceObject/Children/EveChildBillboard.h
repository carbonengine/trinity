////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildBillboard_H
#define EveChildBillboard_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( EveSpaceObject2 );

BLUE_CLASS( EveChildBillboard ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveChildBillboard( IRoot* lockobj = NULL );
	~EveChildBillboard();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent );
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent );
	void GetLocalToWorldTransform( Matrix& transform ) const;

	void PlayCurveSet( const std::string& name ) {};
	void StopCurveSet( const std::string& name ) {};
	float GetCurveSetDuration( const std::string& name ) const { return 0; }

	virtual void Transform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation ) { EveChildTransform::Transform( scale, rotation, translation ); }
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

private:
	BlueSharedString m_name;
	Tr2MeshBasePtr m_mesh;
	Vector4 m_boundingSphere;

	bool m_display;
};

TYPEDEF_BLUECLASS( EveChildBillboard );

#endif