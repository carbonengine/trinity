#pragma once
#ifndef EvePlanet_h
#define EvePlanet_h


#include "Tr2DeviceResource.h"
#include "IWorldPosition.h"
#include "include/ITriFunction.h"
#include "include/ITriTargetable.h"
#include "TriObserverLocal.h"
#include "Tr2ShLightingManager.h"

BLUE_DECLARE( EvePlanet );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE_VECTOR( EvePlanet );
BLUE_DECLARE( EveTransform );

class TriFrustum;
struct ITr2Renderable;
struct ViewDistanceInfo;

BLUE_CLASS( EvePlanet ):
	public IWorldPosition,
	public ITr2SecondaryLightSource,
	public Tr2DeviceResource,
	public ITriTargetable
{
public:
    EXPOSE_TO_BLUE();
    EvePlanet( IRoot* lockobj = NULL );
	~EvePlanet();

	void Update( EveUpdateContext& updateContext );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	void UpdateZOnlyVisibility( const TriFrustum& frustum );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	void GetZOnlyRenderables( std::vector<ITr2Renderable*>& renderables );
	void SetRenderScale( float value );

	static const float SCALE;

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	virtual const Vector3* GetWorldPosition();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2SecondaryLightSource
	virtual void RegisterSecondaryLightSource( Tr2ShLightingManager& );
	virtual void UnregisterSecondaryLightSource( Tr2ShLightingManager& );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriTargetable
	unsigned int GetDamageLocatorCount() const;
	int GetClosestDamageLocatorIndex( const Vector3* position );
	bool GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace );
	bool GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetGoodDamageLocatorIndex( const Vector3& position );
	float GetRadius() const;
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	bool UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex );
	bool GetImpactPosition( Vector3& out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon );
	bool HasImpactConfigurationShield() const;

private:
	bool OnPrepareResources();

	// get global loding thresholds
	float GetVisibilityThreshold() const;
	float GetMediumDetailThreshold() const;

	// calc pixel diameters
	float EstimatePixelDiameterPos( const Vector3* scaledPlanetCenter, float tanFOV, float scale ) const;
	float EstimatePixelDiameterDist( float scaledDistance, float tanFOV, float scale ) const;

	// calc current texture size
	int CalcRequiredTextureSize( float maxDiameter );

	// resource updates
	bool RequiresResourceProcessing() const;

	std::string m_name;

	bool m_display;
	bool m_update;

	bool m_needResources;			

	bool m_warpMode;

	int m_currentTextureSize;		// Heightmap texture size being used for this planet
	int m_requiredTextureSize;		// Heightmap texture size the planet needs

	BlueScriptCallback m_pythonResourceCallback;

	float m_estimatedPixelDiameter;
	float m_estimatedMaxPixelDiameter;

	float m_renderScale;

	float m_scaling;
	float m_radius;
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;

	Matrix m_worldTransform;

	Color m_albedoColor;
	Color m_emissiveColor;

	EveTransformPtr m_highDetail;
	EveTransformPtr m_zOnlyModel;
	
	// PlacementObservers
	PTriObserverLocalVector m_observers;

	void PrepareForWarp(float minDist, const Vector3& dest);
	void WarpStopped();
};

TYPEDEF_BLUECLASS( EvePlanet );
#endif //EvePlanet_h