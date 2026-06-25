// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EvePlanet_h
#define EvePlanet_h

#include "IWorldPosition.h"
#include "TriObserverLocal.h"
#include "Tr2ShLightingManager.h"
#include "SpaceObject/Children/EveChildMesh.h"
#include "EveEffectRoot2.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

BLUE_DECLARE( EvePlanet );
BLUE_DECLARE_VECTOR( EvePlanet );
BLUE_DECLARE( EveTransform );
BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );
BLUE_DECLARE( EveEffectRoot2 );
BLUE_DECLARE_VECTOR( IWorldPosition );

class TriFrustum;
struct ITr2Renderable;
struct ViewDistanceInfo;
class EveUpdateContext;


BLUE_CLASS( EvePlanet ) :
	public EveEffectRoot2
{
public:
	EXPOSE_TO_BLUE();
	EvePlanet( IRoot* lockobj = NULL );
	~EvePlanet();

	void UpdatePlanetSyncronous( const EveUpdateContext& updateContext, float renderScale );
	void UpdatePlanetVisibility( const EveUpdateContext& updateContext, float renderScale );
	void UpdateZOnlyVisibility( const EveUpdateContext& updateContext );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	void GetZOnlyRenderables( std::vector<ITr2Renderable*> & renderables );

	void UpdateLOD();
	void SetRenderScale( float value );

	float GetEstimatedPixelDiameter();
	ITriVectorFunctionPtr GetTranslationCurve();

	static const float SCALE;

	// IWorldPosition
	virtual Vector3 GetWorldPosition();
	virtual Quaternion GetWorldRotation();

	// ITr2SecondaryLightSource
	virtual void RegisterSecondaryLightSource( Tr2ShLightingManager& );
	virtual void UnregisterSecondaryLightSource( Tr2ShLightingManager& );
	void UpdateEffectChildren( const EveUpdateContext& updateContext, Matrix& worldTransform, float renderScale );

	// ITriTargetable
	unsigned int GetDamageLocatorCount() const;
	int GetClosestDamageLocatorIndex( const Vector3* position );
	bool GetDamageLocatorPosition( Vector3 * out, int index, bool inWorldSpace );
	bool GetDamageLocatorDirection( Vector3 * out, int index, bool inWorldSpace );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetGoodDamageLocatorIndex( const Vector3& position );
	float GetRadius() const;
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	bool UpdateImpact( Vector3 & out, const Vector3& direction, int impactIndex );
	bool GetImpactPosition( Vector3 & out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon );
	bool HasImpactConfigurationShield() const;

	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer );

private:
	Matrix CalculatePlanetScaleTransform( const Matrix& worldTransform, float renderScale ) const;
	bool OnPrepareResources();

	// calc pixel diameters
	float EstimatePixelDiameterPos( const Vector3* scaledPlanetCenter, float tanFOV, float scale ) const;
	float EstimatePixelDiameterDist( float scaledDistance, float tanFOV, float scale ) const;

	// calc current texture size
	int CalcRequiredTextureSize( float maxDiameter );
	void SetLod( Tr2Lod lod );

	bool m_update;
	float m_estimatedPixelDiameter;
	float m_estimatedMaxPixelDiameter;
	float m_renderScale;
	float m_radius;
	float m_minScreenSize;
	Color m_albedoColor;
	Color m_emissiveColor;
	EveChildMeshPtr m_zOnlyModel;

	using EveEffectRoot2::GetRenderables; // Silence warning about this hidden function
};

TYPEDEF_BLUECLASS( EvePlanet );
#endif //EvePlanet_h
