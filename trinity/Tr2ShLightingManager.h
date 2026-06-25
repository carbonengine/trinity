// Copyright © 2014 CCP ehf.

#pragma once
#ifndef Tr2ShLightingManager_H
#define Tr2ShLightingManager_H

class EveUpdateContext;
BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE_VECTOR( Tr2PointLight );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ShLightingManager is a class for computing SH coefficients to approximate.
//   Currently it only deals with secondary lighting: lighting from a single primary
//   light source reflected from "secondary light sources", i.e. spheres with set albedo
//   color and radius. Additionally these "secondary light sources" can have a self-
//   emissive color.
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ShLightingManager ) :
	public IRoot
{
public:
	Tr2ShLightingManager( IRoot* lockobj = NULL );
	~Tr2ShLightingManager();

	EXPOSE_TO_BLUE();

	// ----------------------------------------------------------------------------------
	// Description:
	//   SH lighting order (quality).
	// ----------------------------------------------------------------------------------
	enum Quality
	{
		L1,
		L2,
	};

	void RegisterSecondaryLightSource(
		const Vector3* position,
		const float* radius,
		const Color* albedo,
		const Color* emissive );
	void UnregisterSecondaryLightSource( const Vector3* position );

	void GetLighting( const Vector3& position, float intensity, float cutoffRadius, Vector4* result );

	void UpdateWithDirectionalLight( const Vector3& direction, const Vector3& color );

	static const size_t PACKED_COEFFICIENT_COUNT = 7;

private:
	struct Source
	{
		const Vector3* position;
		const float* radius;
		const Color* albedo;
		const Color* emissive;
	};
	struct SourceData
	{
		Vector3 position;
		float radius;
		Vector3 albedo;
		float cutoffMultiplier;
		Vector3 emissive;
		float maxColorComponent;
	};

	void UpdateSourceData();
	template <int>
	void CalculateSecondaryLighting( const Vector3& position, float intensity, float cutoffRadius, Vector4* lightingCoefficients );

	// Registered secondary light sources
	TrackableStdVector<Source> m_sources;

	// Processed and packed secondary light source data
	CcpAlignedMallocBuffer m_sourceData;
	// Number of light sources in m_sourceData
	size_t m_sourceCount;

	// Directional light direction
	Vector3 m_sunDirection;
	// Directional light color
	Vector3 m_sunColor;

	// Overall intensity for secondary SH lighting
	float m_secondaryIntensity;
	// Overall intensity for primary SH lighting (from a list of point lights)
	float m_primaryIntensity;
	// SH lighting order
	Quality m_quality;

	PTr2PointLightVector m_lights;
};

TYPEDEF_BLUECLASS( Tr2ShLightingManager );

// --------------------------------------------------------------------------------------
// Description:
//   Helper interface for secondary light sources that can be used with
//   Tr2ShLightingManager.
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2SecondaryLightSource ) :
	public IRoot
{
	virtual void RegisterSecondaryLightSource( Tr2ShLightingManager& ) = 0;
	virtual void UnregisterSecondaryLightSource( Tr2ShLightingManager& ) = 0;
};

// --------------------------------------------------------------------------------------
// Description:
//   Helper interface for secondary light receiver that can be used with
//   Tr2ShLightingManager.
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2ShLightingReceiver ) :
	public IRoot
{
	virtual void UpdateShLighting( Tr2ShLightingManager&, const EveUpdateContext& ) = 0;
	virtual void ClearShLighting() = 0;
};

BLUE_DECLARE_IVECTOR( ITr2ShLightingReceiver );


#endif