////////////////////////////////////////////////////////////
//
//    Created:   2016
//    Copyright: CCP 2016
//
#pragma once
#ifndef EveTacticalOverlay_H
#define EveTacticalOverlay_H

#include "Eve/IEveSpaceObject2.h"
#include "include/ITriFunction.h"

BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( EveTacticalOverlayTrackObject ):
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveTacticalOverlayTrackObject( IRoot* lockobj = NULL );
	~EveTacticalOverlayTrackObject() {}

	void UpdatePosition( EveUpdateContext& updateContext );
	//void CalculatePixelSize( const TriFrustum& frustum );

	inline Vector3 GetPosition() { return m_position; }



private:
	ITriVectorFunctionPtr m_positionCurve;
	Vector3 m_position;
	//float m_lod;
};
TYPEDEF_BLUECLASS( EveTacticalOverlayTrackObject );
BLUE_DECLARE_VECTOR( EveTacticalOverlayTrackObject );

BLUE_CLASS( EveTacticalOverlay ) :
	public IEveSpaceObject2,
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveTacticalOverlay( IRoot* lockobj = NULL );
	~EveTacticalOverlay();
	//using IEveSpaceObject2::Lock;
	//using IEveSpaceObject2::Unlock;

	struct SphereConnectorVertex
	{
		//Vector3 targetPosition;
		//Vector2 miscData;
		Vector4 instanceData;
		static const Tr2VertexDefinition& GetDefinition();
	};

	struct AnchorVertex
	{
		Vector3 position;
		static const Tr2VertexDefinition& GetDefinition();
	};
	
	// IInitialize
	bool Initialize();
	// INotify
	bool OnModified( Be::Var* value );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext ) {}
	void RenderDebugInfo( Tr2RenderContext& renderContext ) {}
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const { sphere = Vector4( 0, 0, 0, 100000 ); return true; }

	// This version of the function should perform an update on the model / ball position
	void GetModelCenterWorldPosition( Vector3 &position, Be::Time t ) { position = Vector3( 0, 0, 0 ); }

	// This version of the function should not update the object
	void GetCurrentModelCenterWorldPosition( Vector3 &position ) { position = Vector3( 0, 0, 0 ); }

	// If possible, return an AABB in local coordinates
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max ) { min = Vector3( -10000, -10000, -10000 ); max = Vector3( 10000, 10000, 10000 ); return true; }

	// Get the local to world transform
	void GetLocalToWorldTransform( Matrix &transform ) const { D3DXMatrixIdentity( &transform ); }

	// Registers an object and its attachments with the quad renderer
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	// Adds quads from space object and its attachments to the quad renderer. ATTENTION: this function is called in-parallel
	virtual void AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer );

private:
	Tr2EffectPtr m_connectorEffect;
	Tr2EffectPtr m_anchorEffect;
	unsigned m_connectorEffectHash;
	unsigned m_anchorEffectHash;

	//std::vector<Vector3> m_positions;
	std::vector<Vector3> m_anchorBuffer;
	std::vector<SphereConnectorVertex> m_connectorBuffer;

	float m_connectorSegmentsLow;
	float m_connectorSegmentsMedium;
	float m_connectorSegmentsHigh;
	float m_totalSegmentsLast;
	float m_requestedSegmentsLast;
	float m_targetSegmentCount;
	float m_arcSegmentMultiplier;
	float m_segmentCountMultiplier;

	// x - active range; y - fadeout length; z - multiplier for range and fadeout
	Vector3 m_ranges;

	PEveTacticalOverlayTrackObjectVector m_trackObjects;
	ITriVectorFunctionPtr m_positionCurve;
	Vector3 m_rootPosition;
	
	// local variable store for passing parameters to effects
	Tr2VariableStorePtr m_variableStore;
	void RegisterVariables();
	void SetVariableStore( Tr2Effect* effect );
};
TYPEDEF_BLUECLASS( EveTacticalOverlay );

#endif