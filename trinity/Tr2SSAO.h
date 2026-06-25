// Copyright © 2022 CCP ehf.

#pragma once

#include "Tr2GpuResourcePool.h"
#include "TriFrustum.h"
#include "ffx_cacao.h"

enum class SSAOQuality
{
	HIGHEST = 0,
	HIGH = 1,
	MEDIUM = 2,
	LOW = 3,
	LOWEST = 4,
};

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2SSAO ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2SSAO( IRoot* lockobj = NULL );

	Tr2GpuResourcePool::Texture Filter( const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, bool temporal );

	void Enable( bool enable );
	void SetQuality( SSAOQuality quality, bool downsampled );

private:
	struct SSAOResources;
	static constexpr unsigned SSAO_PASS_COUNT = 4;

	struct Layer
	{
		bool enabled = true;
		SSAOQuality quality;
		bool downsampled;
		float zoomLevel;

		FFX_CACAO_Settings settings;
		Tr2EffectPtr effect;
	};

	HRESULT ApplyConstBuffer( unsigned pass, Tr2RenderContext& renderContext );
	Tr2GpuResourcePool::Texture PerformPass( const Layer& layer, const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, bool reuseNormals, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );

	Layer m_detail = { true, SSAOQuality::HIGHEST, false, 5.f };

	Tr2ConstantBufferAL m_constBuffers[SSAO_PASS_COUNT + 1]{};


	//CORTAO stuff
	struct CortaoPerObjectData
	{
		Vector4 resolution;

		Vector4 projectionParams;

		Vector4 unprojectParams;

		Vector2 depthParams;
		float radius;
		float normalBias;

		float maxApparentCircleRadiusCoefficient;
		float mipBias;
		float radiusMultiplier;
		float lookupOccluderRadiusScale;

		uint32_t randomVectorSeedX;
		uint32_t randomVectorSeedY;
		float randomAngleOffset;
		float inverseMaxSlopeWeight;

		Matrix normalMatrix;

		uint32_t mipCount;
		uint32_t padding0;
		uint32_t padding1;
		uint32_t padding2;
	};

	struct CortaoDownsamplePerObjectData
	{
		uint32_t width;
		uint32_t height;
		uint32_t mipLevel;
		uint32_t random;
	};

	bool m_cortaoEnabled;
	bool m_cortaoBentNormal;

	bool m_cortaoInitialized;
	Tr2EffectPtr m_cortaoEffect;
	Tr2EffectPtr m_cortaoBlurEffect;
	TriTextureResPtr m_cortaoLookupTable;
	Tr2ConstantBufferAL m_cortaoConstantBuffer;

	float m_cortaoStrength;
	float m_cortaoRadius;
	float m_cortaoMaxBlockerSearchRadius;
	float m_cortaoMipBias;

	bool m_cortaoBlur;


	uint32_t m_cortaoRandSeeds[4];

	uint32_t Hash( uint32_t n );

	Tr2GpuResourcePool::Texture ComputeCORTAO( const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, bool temporal );
};

TYPEDEF_BLUECLASS( Tr2SSAO );
