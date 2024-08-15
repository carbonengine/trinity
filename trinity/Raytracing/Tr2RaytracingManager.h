#pragma once

#include "Tr2RaytracingGeometry.h"
#include "Tr2Denoiser.h"

class Tr2RtShaderTableAL;

BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2RaytracingManager ) : public IRoot
{
public:
	Tr2RaytracingManager( IRoot* lockobj = nullptr );
	~Tr2RaytracingManager();

	EXPOSE_TO_BLUE();

	Tr2RaytracingGeometry& GetGeometry();

	void RenderShadows( ITr2TextureProvider* depth, ITr2TextureProvider* normal, const Vector3& sunDirection, const CcpMath::Sphere* planets, size_t planetCount, Tr2RenderContext& renderContext );
	
	bool OnPrepareResources();
	void ReleaseResources( TriStorage s );

	ITr2TextureProvider* GetShadowMap() const;

private:
		 
	Tr2RaytracingGeometryPtr m_geometry;

	Tr2RaytracingPipelineStateManager m_pipelineManager;

	Tr2EffectPtr m_shadowEffect;
	unsigned m_shadowEffectHash;
	Tr2RtShaderTableAL m_shadowShaderTable;
	Tr2RenderTargetPtr m_destTex;
	Tr2ConstantBufferAL m_shadowPerFrameData;

	// denoiser
	Tr2DenoiserPtr m_denoiser;

	float m_sunAngle;
	// debug
	bool m_applyDenoiser;

	// White texture for no shadow
	TriTextureResPtr m_whiteTexture;
};

TYPEDEF_BLUECLASS( Tr2RaytracingManager );
