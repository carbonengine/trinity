// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveCurveLineSet_H
#define EveCurveLineSet_H

#include "Tr2CurveLineSet.h"
#include "Eve/IEveTransform.h"
#include "Eve/IEveSpaceObject2.h"

BLUE_DECLARE( EveCurveLineSet );

class Tr2PerObjectData;
class Tr2PerObjectDataStandard;

BLUE_CLASS( EveCurveLineSet ) :
	public Tr2CurveLineSet,
	public IEveTransform,
	public IEveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();

	EveCurveLineSet( IRoot* lockobj = NULL );
	~EveCurveLineSet();

	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( const EveUpdateContext& updateContext );
	void UpdateAsyncronous( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveTransform
	void Update( const EveUpdateContext& updateContext );
	Tr2Lod GetLODLevel() const
	{
		return TR2_LOD_HIGH;
	}

	// No sensible implementation?
	void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time t )
	{
	}
	void GetModelCenterWorldPosition( Vector3 & position ) const
	{
	}
	bool GetLocalBoundingBox( Vector3 & min, Vector3 & max )
	{
		return false;
	}
	void GetLocalToWorldTransform( Matrix & transform ) const
	{
		transform = IdentityMatrix();
	}

private:
	bool m_isVisible;
};

TYPEDEF_BLUECLASS( EveCurveLineSet );
BLUE_DECLARE_VECTOR( EveCurveLineSet );

#endif