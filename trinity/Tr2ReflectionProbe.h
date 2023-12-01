////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#pragma once

#include "Tr2DeviceResource.h"
#include <array>
#include "TriFrustum.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE_INTERFACE( ITriTextureRes );

BLUE_CLASS( Tr2ReflectionProbe ) :
	public INotify,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	enum ReflectionProbeRenderFrequency
	{
		ONE_SIDE_PER_FRAME,
		ALL_SIDES_PER_FRAME
	};

	Tr2ReflectionProbe( IRoot* lockobj = NULL );
	~Tr2ReflectionProbe();

	void ReleaseResources( TriStorage s );
	bool OnPrepareResources();

	bool IsValid();
	bool HasData() const;
	void InitRenderPass( Tr2RenderContext &renderContext );
	void StartRenderFace( unsigned face, Tr2RenderContext &renderContext );
	void EndRenderPass( Tr2RenderContext &renderContext );

	TriFrustum GetFrustum( unsigned face, Tr2RenderContext& renderContext );

	Tr2RenderTargetPtr GetReflection();
	void SetPosition( Vector3 position );

	void SetBackLightColor( Color color );
	void SetIntensity( float intensity);
	void SetBackLightContrast( float contrast );

	bool OnModified( Be::Var* value );

	bool IsHollyWoodModeOn() const;
	bool ReadyForDynamicObjectReflections() const;

	uint8_t GetStartFace() const;
	uint8_t GetEndFace() const;

private:
	void RunFilter();
	void Filter( Tr2RenderContext &renderContext );

	bool m_initialized;
	bool m_hasData;
	bool m_lockPosition;
	Vector3 m_position;
	int m_intermediateSize;

	std::array<Tr2RenderTargetPtr, 6> m_renderTargets;
	std::array<Tr2TextureAL, 6> m_stencilMaps;
	Tr2RenderTargetPtr m_renderTargetCube;

	Tr2EffectPtr m_preFilterEffect;
	Tr2EffectPtr m_filterEffect;
	Tr2RenderTargetPtr m_preFilterTarget;
	Tr2RenderTargetPtr m_postFilterTarget;

	ITriTextureResPtr m_customSourceTexture;

	bool m_prevCullInversion;
	bool m_hdrOutput;
	ReflectionProbeRenderFrequency m_renderFrequency;
	uint8_t m_currentFrame;
	bool m_onePassDone;

	// Controls for hollywood lighting
	bool m_hollywoodMode;
	Color m_backlightColor;
	float m_backlightContrast;
};

TYPEDEF_BLUECLASS( Tr2ReflectionProbe );
