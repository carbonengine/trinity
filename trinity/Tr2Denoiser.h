////////////////////////////////////////////////////////////
//
//    Created:   January 2020
//    Copyright: CCP 2020
//

#pragma once
BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2Effect );


BLUE_CLASS( Tr2Denoiser ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2Denoiser();

	ALResult Apply( ITr2TextureProvider & source, ITr2TextureProvider & depth, ITr2TextureProvider * normals, const Matrix& projection, Tr2RenderContext& renderContext );
	ITr2TextureProvider* GetTexture() const;

	bool OnModified( Be::Var * value ) override;

private:
	Tr2RenderTargetPtr m_noiseEstimate;
	Tr2RenderTargetPtr m_intermediate;
	Tr2RenderTargetPtr m_result;

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