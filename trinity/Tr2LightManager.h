// Copyright © 2014 CCP ehf.

#pragma once
#ifndef Tr2LightManager_H
#define Tr2LightManager_H

#include "Tr2DeviceResource.h"
#include "Tr2Variable.h"
#include "TriFrustum.h"
#include "Tr2AddSafeGrowableBuffer.h"
#include "TbbStub.h"

#include "Raytracing/Tr2RaytracingGeometry.h"
#include "Tr2Denoiser.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( Tr2GpuStructuredBuffer );
BLUE_DECLARE( Tr2DepthStencil );

BLUE_DECLARE( TriTextureRes );

class Tr2TextureArray;

enum class ShadowQuality
{
	SHADOW_DISABLED,
	SHADOW_LOW,
	SHADOW_HIGH,
	SHADOW_RAYTRACED
};

// --------------------------------------------------------------------------------------
// Description:
//   Manages light sources for tiled/clustered lighting. An object of this class does not
//   store lights permanently. Rather a user adds lights every frame and then the object
//   updates GPU buffers needed for tiled lighting. Normally the per-frame operations
//   for the light manager would look like:
//     lightManager.Clear();
//     lightManager.SetFrustum( frustum );
//     for each in scene.lights
//       lightManager.AddPointLight( each ); // this can be done in parallel
//	   lightManager.ResolveLightData();
//     lightManager.UpdateLists();
//     renderYourStuff();
//  Light manager GPU buffers are accessed through variable store with names
//  "LightBuffer" and "LightIndexBuffer".
// --------------------------------------------------------------------------------------
class Tr2LightManager : public Tr2DeviceResource
{
public:
	Tr2LightManager( const char* effectPath );
	~Tr2LightManager();

	struct PerLightData
	{
		Vector3 position;
		float radius;

		Vector3 color;
		Float_16 innerRadius;
		uint16_t flags;

		Vector3_16 direction;
		Float_16 projectionPlaneDistance;

		Float_16 outerAngle;
		Float_16 innerAngle;

		union
		{
			struct
			{
				uint32_t padding : 2;
				uint32_t shadowMapScale : 10;
				uint32_t shadowMapOffsetX : 10;
				uint32_t shadowMapOffsetY : 10;
			} ShadowMapping;
			struct
			{
				uint16_t shadowMask;
				uint16_t padding;
			} Raytracing;
		};
	};

	struct ShadowMapAtlasSettings
	{
		uint32_t actualTextureSize; // only updated once per frame. might be larger than size, as multiple eve space scenes might request different shadow qualities
		uint32_t sizeLog2;
		uint32_t size;
		uint32_t entryMinSizeLog2;
		uint32_t entryMinSize;
		uint32_t entryInverseScaleFactorLog2;
		uint32_t entryMaxSize;
	};

	void GetUnpackedShadowMapData( const PerLightData& lightData, uint32_t& shadowMapScale, uint32_t& shadowMapOffsetX, uint32_t& shadowMapOffsetY ) const;

	static const uint16_t FLAG_AFFECTS_SURFACES = 1;
	static const uint16_t FLAG_AFFECTS_PARTICLES = 1 << 1;
	static const uint16_t FLAG_CASTS_SHADOWS = 1 << 2;
	static const uint16_t FLAG_IS_VOLUMETRIC = 1 << 3;

	static const uint16_t FLAG_DEFAULT = FLAG_AFFECTS_SURFACES;

	void Clear( Tr2RenderContext& renderContext );
	void SetFrustum( const TriFrustum& frustum );
	void AddPointLight( const Vector3& position, float radius, const Color& color, Float_16 innerRadius = Float_16( 0.f ), uint16_t flags = FLAG_DEFAULT );
	void AddLight( PerLightData& data );
	void ResolveLightData();
	ALResult UpdateLists( const Tr2TextureAL& depthMap, Tr2RenderContext& renderContext );
	void SetVariableStore();
	void AdjustLightCutoff( float lodFactor );

	void SetShadowQuality( ShadowQuality shadowQuality, uint64_t frameCounter );

	virtual void ReleaseResources( TriStorage s );

	static void ResetVariableStore();
	static Tr2LightManager* GetOrCreateInstance( const char* effectPath );
	static Tr2LightManager* GetInstance();
	static void DeleteInstance();

	static bool AreLightFlagsValid( uint16_t flags );

	static Tr2TextureArray& GetLightProfileArray();

	const std::vector<uint32_t>& GetShadowCastingLights() const;
	const std::vector<uint32_t>& GetVolumetricLights() const;
	const Tr2LightManager::PerLightData& GetLightData( uint32_t index ) const;
	Tr2GpuResourcePool::Texture GetShadowMapAtlas( Tr2GpuResourcePool& gpuResourcePool );
	const ShadowMapAtlasSettings& GetShadowMapAtlasSettings() const;

	ShadowQuality GetCurrentSpaceSceneShadowQuality();

	Tr2GpuResourcePool::Texture RenderRaytracedShadows(
		Tr2RaytracingGeometry* geometry,
		const Tr2TextureAL& depth,
		const Tr2TextureAL& normal,
		const CcpMath::Sphere* planets,
		size_t planetCount,
		Tr2GpuResourcePool& gpuResourcePool,
		Tr2RenderContext& renderContext );
	Tr2RaytracingPipelineStateManager* GetRaytracingPipelineManager();
	Tr2RtShaderTableDescriptionAL* GetRaytracingShaderTableDesc();

private:
	Tr2LightManager( const Tr2LightManager& );
	Tr2LightManager& operator=( const Tr2LightManager& );

	struct AtlasNode
	{
		uint32_t children[2];
		uint32_t lightIndex;
		int x;
		int y;
		int width;
		int height;
	};

	// used for filtering out shadowcasting and volumetric lights
	struct LightScreenSizeTuple
	{
		uint32_t lightIndex;
		float sizeAcross;
	};

	virtual bool OnPrepareResources();

	ALResult DoUpdateLists( const Tr2TextureAL& depthMap, Tr2RenderContext& renderContext );
	ALResult ClearLightIndices( Tr2RenderContext& renderContext );
	ALResult UpdateLightBuffer( Tr2RenderContext& renderContext );

	uint32_t InsertAtlasNode( std::vector<AtlasNode>& atlasNodes, uint32_t nodeId, uint32_t lightIndex, int32_t width, int32_t height );

	bool GetShadowMapAtlasEntry( uint32_t lightIndex, uint32_t width, uint32_t height, uint32_t& out_posX, uint32_t& out_posY );
	void CreateShadowMapAtlas( uint32_t numShadowCastingLights, const std::vector<LightScreenSizeTuple>& lightTuples );

	Tr2EnumerableThreadSpecific<std::vector<PerLightData>> m_tlsLightData;
	std::vector<PerLightData> m_lightData;
	std::vector<uint32_t> m_shadowCastingLights;
	std::vector<uint32_t> m_volumetricLights;
	Tr2GpuStructuredBufferPtr m_lightBuffer;
	Tr2GpuStructuredBufferPtr m_indexBuffer;

	Tr2GpuBufferPtr m_indexBufferCounter;
	Tr2ConstantBufferAL m_perFrameData;
	Tr2EffectPtr m_effect;
	Tr2Variable m_lightBufferVariable;
	Tr2Variable m_indexBufferVariable;
	TriFrustum m_frustum;

	float m_adjustedCutoff;

	uint32_t nextFrameShadowQuality; // bitmask, collecting ShadowQualities during the current frame
	ShadowQuality m_currentSpaceSceneShadowQuality;
	uint64_t m_currentFrameCounter;

	struct
	{
		ShadowMapAtlasSettings m_atlasSettings;
		std::vector<AtlasNode> m_atlasNodes;
		ShadowQuality m_qualityUsedByAtlas;
	} m_ShadowMap;

	struct
	{
		Tr2RaytracingPipelineStateManager m_pipelineManager;
		Tr2RtShaderTableDescriptionAL m_shaderTableDesc;

		Tr2RaytracingGeometryPtr m_geometry;

		Tr2EffectPtr m_effect;
		unsigned m_effectHash;
		Tr2RtShaderTableAL m_shaderTable;
		Tr2ConstantBufferAL m_perFrameData;
	} m_Raytracing;
};

#endif
