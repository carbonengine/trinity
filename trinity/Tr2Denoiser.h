// Copyright © 2020 CCP ehf.

#pragma once

#include "Tr2GpuResourcePool.h"

BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2Effect );


BLUE_CLASS( Tr2Denoiser ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2Denoiser();

	Tr2GpuResourcePool::Texture Apply( Tr2GpuResourcePool::Texture source, const Tr2TextureAL& depth, const Tr2TextureAL& normals, const Matrix& projection, float upscaling, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );
	Tr2GpuResourcePool::Texture Apply( Tr2GpuResourcePool::Texture source, const Tr2TextureAL& depth, const Tr2TextureAL& normals, const Matrix& projection, uint32_t index, float upscaling, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );

	void SetRadius( uint32_t value );

	bool OnModified( Be::Var * value ) override;

private:
	Tr2EffectPtr m_estimateNoise;
	Tr2EffectPtr m_denoiseEstimate;
	Tr2EffectPtr m_denoiseHoriz;
	Tr2EffectPtr m_denoiseVert;

	uint32_t m_radius;
	uint32_t m_stepSize;

	float m_depthWeight;
	float m_normalWeight;
	float m_planeWeight;

	bool m_bypass;
	bool m_parametersDirty;
};

TYPEDEF_BLUECLASS( Tr2Denoiser );