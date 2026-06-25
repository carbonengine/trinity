// Copyright © 2023 CCP ehf.

#pragma once

#include "Tr2RaytracingGeometry.h"
#include "Tr2Denoiser.h"
#include "Tr2ShadowMap.h"

class Tr2RtShaderTableAL;

BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2RaytracingManager ) :
	public IRoot
{
public:
	Tr2RaytracingManager( IRoot* lockobj = nullptr );
	~Tr2RaytracingManager();

	EXPOSE_TO_BLUE();

	Tr2RaytracingGeometry& GetGeometry();

	Tr2GpuResourcePool::Texture RenderShadows(
		const Tr2TextureAL& depth,
		const Tr2TextureAL& normal,
		const Vector3& sunDirection,
		const CcpMath::Sphere* planets,
		size_t planetCount,
		float upscaling,
		Tr2GpuResourcePool& gpuResourcePool,
		Tr2RenderContext& renderContext );

	bool OnPrepareResources();
	void ReleaseResources( TriStorage s );

	Tr2RaytracingPipelineStateManager m_pipelineManager;
	Tr2RtShaderTableDescriptionAL m_shaderTableDesc;

private:
	Tr2RaytracingGeometryPtr m_geometry;


	Tr2EffectPtr m_shadowEffect;
	unsigned m_shadowEffectHash;
	Tr2RtShaderTableAL m_shadowShaderTable;
	Tr2ConstantBufferAL m_shadowPerFrameData;

	// denoiser
	Tr2DenoiserPtr m_denoiser;

	float m_sunAngle;
	// debug
	bool m_applyDenoiser;
};

TYPEDEF_BLUECLASS( Tr2RaytracingManager );
