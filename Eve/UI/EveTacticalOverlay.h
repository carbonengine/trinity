////////////////////////////////////////////////////////////
//
//    Created:   2016
//    Copyright: CCP 2016
//
#pragma once
#ifndef EveTacticalOverlay_H
#define EveTacticalOverlay_H

#include "Eve/IEveSpaceObject2.h"
#include "Tr2QuadRenderer.h"
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
	inline Vector3 GetVelocity() const { return m_velocity; }
	inline Vector3 GetPosition() const { return m_position; }
	inline float GetRadius() const { return m_radius; }
	inline bool IsAggressive() const { return m_aggressive; }
	inline bool ShowVelocity() const { return m_showVelocity; }

private:
	ITriVectorFunctionPtr m_positionCurve;
	Vector3 m_position;
	Vector3 m_velocity;
	float m_radius;
	bool m_aggressive;
	bool m_showVelocity;
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

	struct SphereConnectorVertex
	{
		Vector4 instanceData;
		float instanceData2;
		static const Tr2VertexDefinition& GetDefinition();
	};

	struct AnchorVertex
	{
		Vector4 instanceData;
		static const Tr2VertexDefinition& GetDefinition();
	};
	
	struct VelocityConnectorVertex
	{
		Vector4 instanceData;
		Vector4 instanceData2;
		static const Tr2VertexDefinition& GetDefinition();
	};
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext ) {}
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors ){};
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const { return false; }
	void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t ) { position = Vector3( 0.f, 0.f, 0.f ); }
	void GetModelCenterWorldPosition( Vector3 &position ) const { position = Vector3( 0.f, 0.f, 0.f ); }
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max ) { return false; }
	void GetLocalToWorldTransform( Matrix &transform ) const { transform = IdentityMatrix(); }

	// Registers an object and its attachments with the quad renderer
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	// Adds quads from space object and its attachments to the quad renderer. ATTENTION: this function is called in-parallel
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

private:
	Tr2EffectPtr m_connectorEffect;
	Tr2EffectPtr m_anchorEffect;
	Tr2EffectPtr m_velocityEffect;
	Tr2QuadRenderer::EffectKey m_connectorEffectHash;
	Tr2QuadRenderer::EffectKey m_anchorEffectHash;
	Tr2QuadRenderer::EffectKey m_velocityEffectHash;

	std::vector<AnchorVertex> m_anchorBuffer;
	std::vector<SphereConnectorVertex> m_connectorBuffer;
	std::vector<VelocityConnectorVertex> m_velocityBuffer;

	float m_connectorSegmentsLow;
	float m_connectorSegmentsMedium;
	float m_connectorSegmentsHigh;
	float m_totalSegmentsLast;
	float m_requestedSegmentsLast;
	float m_targetSegmentCount;
	float m_arcSegmentMultiplier;
	float m_segmentCountMultiplier;

	float m_interestRange;
	float m_outsideInterestIntensity;
	
	// minimum radius of an object for us to bother drawing a radius line for it
	float m_minRadiusForRange;
	// x - active range; y - fadeout length; z - multiplier for range and fadeout; w - source radius
	Vector4 m_ranges;

	PEveTacticalOverlayTrackObjectVector m_trackObjects;
	EveTacticalOverlayTrackObjectPtr m_interestObject;
	ITriVectorFunctionPtr m_positionCurve;
	Vector3 m_rootPosition;
	Vector3 m_rootVelocity;
	
	// local variable store for passing parameters to effects
	Tr2VariableStorePtr m_variableStore;
	void RegisterVariables();
	void SetVariableStore( Tr2Effect* effect );
};
TYPEDEF_BLUECLASS( EveTacticalOverlay );

#endif