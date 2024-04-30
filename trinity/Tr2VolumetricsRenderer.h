////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#pragma once

#include "ITr2VolumetricRenderable.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( EveComponentRegistry );
class TriFrustum;
class ITriRenderBatchAccumulator;


BLUE_CLASS( Tr2VolumetricsRenderer ) :
	public IRoot
{
public:
	Tr2VolumetricsRenderer( IRoot* lockobj = nullptr );

	void RenderVolumetrics(
		const EveComponentRegistry& registry,
		const TriFrustum& frustum,
		Tr2DepthStencil& sceneDepth,
		const Vector3& sunDirection,
		const float depthSlices[4],
		Tr2RenderContext& renderContext );
	void UpdateVariableStore();
	void RenderShadows(
		const EveComponentRegistry& registry,
		ITr2TextureProvider* shadowMap,
		Tr2RenderContext& renderContext );

	EXPOSE_TO_BLUE();

private:
	Tr2EffectPtr m_volumeBlit;
	Tr2EffectPtr m_downsampleDepth;
	Tr2EffectPtr m_hBlur;
	Tr2EffectPtr m_vBlur;
	Tr2TextureReferencePtr m_volumeSlices;
	Tr2RenderTargetPtr m_downsampledDepth;
	Tr2RenderTargetPtr m_blurScratch;
	std::unique_ptr<ITriRenderBatchAccumulator> m_batches;
	Tr2VolumerticQuality m_quality;
	float m_scaleFactor;
	bool m_blur;
	bool m_volumeHasContent;
	bool m_castShadows;
	bool m_receiveShadows;
};

TYPEDEF_BLUECLASS( Tr2VolumetricsRenderer );
