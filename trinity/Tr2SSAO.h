////////////////////////////////////////////////////////////////////////////////
//
// Created:   March 2022
// Copyright: CCP 2022
//

#pragma once

#include "Tr2DeviceResource.h"
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
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE_INTERFACE( ITriTextureRes );

BLUE_CLASS( Tr2SSAO ) :
	public INotify,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2SSAO( IRoot* lockobj = NULL );
	~Tr2SSAO();

	void ReleaseResources( TriStorage s );
	bool OnPrepareResources();

	bool IsValid();

	bool OnModified( Be::Var* value );

	void SetInputBuffers( Tr2DepthStencilPtr depthBuffer, Tr2RenderTargetPtr normalBuffer );

	void Filter( Tr2RenderContext& renderContext );

	Tr2RenderTargetPtr GetOutput() const;
	ITr2TextureProviderPtr GetBlankOutput() const;

private:
	struct SSAOResources;
	static constexpr unsigned SSAO_PASS_COUNT = 4;


	struct SSAOResources
	{
		SSAOResources();
		void ReleaseResources();

		// Prepare
		Tr2TextureAL deinterleavedDepthTexture;
		Tr2TextureAL deinterleavedNormalTexture;
		Tr2RenderTargetPtr deinterleavedDepthTarget;
		Tr2RenderTargetPtr deinterleavedNormalTarget;

		// SSAO
		Tr2TextureAL ssaoWorkerTextureA;
		Tr2TextureAL ssaoWorkerTextureB;
		Tr2RenderTargetPtr ssaoWorkerTargetA;
		Tr2RenderTargetPtr ssaoWorkerTargetB;

		// Importance
		Tr2RenderTargetPtr importanceTargetA;
		Tr2RenderTargetPtr importanceTargetB;
	};

	struct Layer
	{
		void ReleaseResources();

		bool enabled = true;
		SSAOQuality quality;
		bool downsampled;
		float zoomLevel;
	
		FFX_CACAO_Settings settings;
		Tr2EffectPtr effect;
		SSAOResources resources;
	};

	HRESULT ApplyConstBuffer( unsigned pass, Tr2RenderContext& renderContext );
	void PerformPass( const Layer& layer, bool reuseNormals, Tr2RenderContext& renderContext );
	void UpdateEffect( Layer& layer );
	bool PrepareSsaoResources( Layer& layer, const Layer* prevLayer, Tr2PrimaryRenderContext& renderContext );

	Layer m_detail = { true, SSAOQuality::HIGHEST, false, 5.f };

	bool m_initialized = false;

	Tr2ConstantBufferAL m_constBuffers[SSAO_PASS_COUNT + 1]{};

	// Input
	Tr2DepthStencilPtr m_inputDepthBuffer;
	Tr2RenderTargetPtr m_inputNormalBuffer;

	Tr2GpuBufferPtr m_loadCounterBuffer;

	// Output
	TriTextureResPtr m_blankOutputTexture;
	Tr2RenderTargetPtr m_outputTarget;
};

TYPEDEF_BLUECLASS( Tr2SSAO );
