////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#pragma once

#include "ITr2VolumetricRenderable.h"
#include "TriFrustumOrtho.h"
#include "Tr2ShadowMap.h"
#include "PostProcess/Tr2PostProcessEnums.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( EveComponentRegistry );
class TriFrustum;
class ITriRenderBatchAccumulator;


BLUE_INTERFACE( ITr2FroxelFogSettings ) :
	public IRoot
{
public:
	struct FroxelFogSettings
	{
		float thickness = 0.f;
		float directionality = 0.f;

		float environmentIntensity = 0.f;

		Color fogColor = Color( 0.0f, 0.0f, 0.0f, 0.0f );
		Color backgroundColor = Color( 0.0f, 0.0f, 0.0f, 0.0f );

		FroxelFogSettings operator*( float rhs ) const;
		FroxelFogSettings operator+( const FroxelFogSettings& rhs ) const;
	};

	struct FroxelFogWeightedSettings
	{
		PostProcessEnums::Priority priority = PostProcessEnums::MEDIUM_PRIORITY;
		float intensity = 1;
		FroxelFogSettings value;
	};
	virtual FroxelFogWeightedSettings GetFroxelFogSettings() = 0;
};

REGISTER_COMPONENT_TYPE( "FroxelFogSettings", ITr2FroxelFogSettings );


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


	void UpdateFogSettings( const EveComponentRegistry& registry );
	bool HasFog() const;

	void RenderFog( 
		Tr2RenderContext & renderContext, 
		uint32_t width, 
		uint32_t height, 
		Tr2ShadowMap * cascadedShadowMap, 
		const Vector3& sunDirection, 
		const Color& sunColor, 
		const Matrix& view, 
		const Matrix& projection, 
		const Matrix& viewLast, 
		const Matrix& projectionLast );
	void RenderFogIntoReflectionMap( 
		Tr2RenderContext& renderContext, 
		uint32_t width, 
		uint32_t height, 
		const Vector3& sunDirection, 
		const Color& sunColor, 
		const Matrix& view, 
		const Matrix& projection );
	void UpdateFogEnvironmentMap( Tr2RenderContext & renderContext );

	void UpdateVariableStore();
	void RenderShadows(
		const EveComponentRegistry& registry,
		ITr2TextureProvider* shadowMap,
		Tr2RenderContext& renderContext );




	EXPOSE_TO_BLUE();

private:
	struct FogViewDependentResources
	{
		explicit FogViewDependentResources( bool temporalFroxels );

		Tr2TextureReferencePtr fogFroxels;
		Tr2TextureReferencePtr temporalFroxels0;
		Tr2TextureReferencePtr temporalFroxels1;

		Tr2EffectPtr calculateFroxels;
		Tr2EffectPtr filterFroxels;
		Tr2EffectPtr raymarchFroxels;
		Tr2EffectPtr applyFroxels;

		Vector3 froxelJitter;
		bool currentTemporalFroxels;
	};

	void RenderFog( 
		FogViewDependentResources& resources, 
		Tr2RenderContext& renderContext, 
		uint32_t originalWidth, 
		uint32_t originalHeight, 
		Tr2ShadowMap* cascadedShadowMap, 
		const Vector3& sunDirection, 
		const Color& sunColor, 
		const Matrix& view, 
		const Matrix& projection, 
		const Matrix& viewLast, 
		const Matrix& projectionLast );

	Tr2EffectPtr m_volumeBlit;
	Tr2EffectPtr m_downsampleDepth;
	Tr2EffectPtr m_hBlur;
	Tr2EffectPtr m_vBlur;
	Tr2TextureReferencePtr m_volumeSlices;
	Tr2RenderTargetPtr m_downsampledDepth;
	Tr2RenderTargetPtr m_blurScratch;

	

	Tr2TextureReferencePtr m_mieEnvironmentMap;
	float m_environmentRandom;
	Vector2 m_environmentJitter;
	float m_environmentDirectionality;
	float m_environmentBlendCounter;

	ITr2FroxelFogSettings::FroxelFogSettings m_froxelFogSettings;

	float m_gameBackClip;
	

	FogViewDependentResources m_fogResources;
	FogViewDependentResources m_fogReflectionResources;

	Tr2EffectPtr m_updateMieEnvironmentMap;

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
