////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildBillboard_H
#define EveChildBillboard_H

#include "IEveSpaceObjectChild.h"
#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( EveSpaceObject2 );

BLUE_CLASS( EveChildBillboard ) :
	public IEveSpaceObjectChild,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	EveChildBillboard( IRoot* lockobj = NULL );
	~EveChildBillboard();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );

	void PlayCurveSet( const std::string& name ) {};
	void StopCurveSet( const std::string& name ) {};
	float GetCurveSetDuration( const std::string& name ) const { return 0; } 
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

private:
	BlueSharedString m_name;

	Vector4 m_boundingSphere;

	Vector3 m_translation;
	Vector3 m_scaling;

	Matrix m_worldTransform;

	Tr2MeshBasePtr m_mesh;

	bool m_display;
};

TYPEDEF_BLUECLASS( EveChildBillboard );

#endif