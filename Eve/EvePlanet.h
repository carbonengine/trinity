#pragma once
#ifndef EvePlanet_h
#define EvePlanet_h


#include "Tr2DeviceResource.h"
#include "IWorldPosition.h"
#include "include/ITriFunction.h"
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
	public Tr2DeviceResource
{
public:
    EXPOSE_TO_BLUE();
    EvePlanet( IRoot* lockobj = NULL );
	~EvePlanet();

	void Update( EveUpdateContext& updateContext );
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	void GetZOnlyRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables );
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

private:
	std::string m_name;

	bool m_display;
	bool m_update;

	bool m_resourcesReady;			// Set when resources are ready for the planet
	bool m_needResources;			
	bool m_resourceActionPending;	// Set when resources are being modified by python
	bool m_forceResourceLoading;	// Indicates that resources should be loaded/created

	bool m_warpMode;

	int m_currentTextureSize;		// Heightmap texture size being used for this planet
	int m_requiredTextureSize;		// Heightmap texture size the planet needs

	BlueScriptCallback m_pythonResourceCallback;

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