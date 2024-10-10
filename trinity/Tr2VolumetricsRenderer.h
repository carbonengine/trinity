////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#pragma once

#include "ITr2VolumetricRenderable.h"
#include "TriFrustumOrtho.h"
#include "Tr2ShadowMap.h"

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

	void RenderFog( Tr2RenderContext & renderContext, Tr2DepthStencil & sceneDepth, Tr2ShadowMap * cascadedShadowMap, Vector3 sunDirection, Color sunColor, Matrix view, Matrix projection, Matrix viewLast, Matrix projectionLast );

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

	
	struct FroxelFogSettings
	{
		float thickness;
		float directionality;

		float environmentIntensity;

		Color fogColor;
		Color backgroundColor;
	};
	FroxelFogSettings m_froxelFogSettings;
	
	Tr2TextureReferencePtr m_fogFroxels;
	Tr2TextureReferencePtr m_temporalFroxels0, m_temporalFroxels1;
	bool m_currentTemporalFroxels;
	Vector3 m_froxelJitter;

	Tr2EffectPtr m_clearFroxels;
	Tr2EffectPtr m_calculateFroxels;
	Tr2EffectPtr m_filterFroxels;
	Tr2EffectPtr m_raymarchFroxels;
	Tr2EffectPtr m_applyFroxels;

	struct FogPerObjectData
	{
		Matrix ProjectionMatrix;
		Matrix InverseProjectionMatrix;

		uint32_t ResolutionX;
		uint32_t ResolutionY;
		uint32_t ResolutionZ;
		float _pad0;

		Vector3 Jitter;
		float Far;

		Vector3 Scattering;
		float BaseDensity;

		float MaxDistanceVisibility;
		float MieG;
		float EnvironmentIntensity;
		float _pad1;

		Vector3 Extinction;
		float _pad2;

		Matrix InverseViewMatrix;

		Vector2 LinearizeDepthParams;
		Vector2 _pad3;

		Vector4 UnprojectParams;
		Vector4 PreviousProjectParams;
		Matrix ReprojectionMatrix;

		Vector3 SunDirection;
		float _pad4;
		Vector3 SunColor;
		float _pad5;

		//Directional light shadows
		Vector4 ShadowMapValues[4]; // x = zFar value[0], y = zFar value[1], z = zFar value[2], w = zFar value[3]..etc
		Matrix ShadowMatrix[16]; // Matrix that takes a coordinate from view space all the way to the packed cascades
		Vector4 SplitInfo; // x = NrOfSplits, y = <unused>, z = <unused>, w = <unused>
	};
	Tr2ConstantBufferAL m_fogConstantBuffer;

	
	std::unique_ptr<ITriRenderBatchAccumulator> m_batches;
	
	Tr2ConstantBufferAL m_shadowPerFrameVSBuffer;
	Tr2VolumerticQuality m_quality;
	float m_scaleFactor;
	bool m_blur;
	bool m_volumeHasContent;
	bool m_castShadows;
	bool m_receiveShadows;
	
};

TYPEDEF_BLUECLASS( Tr2VolumetricsRenderer );
