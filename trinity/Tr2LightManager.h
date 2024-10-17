////////////////////////////////////////////////////////////
//
//    Created:   October 2014
//    Copyright: CCP 2014
//

#pragma once
#ifndef Tr2LightManager_H
#define Tr2LightManager_H

#include "Tr2DeviceResource.h"
#include "Tr2Variable.h"
#include "TriFrustum.h"
#include "Tr2AddSafeGrowableBuffer.h"
#include "TbbStub.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( Tr2GpuStructuredBuffer );
BLUE_DECLARE( Tr2DepthStencil );

class Tr2TextureArray;


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
class Tr2LightManager: public Tr2DeviceResource
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

		uint32_t padding : 2;
		uint32_t shadowMapScale : 10;
		uint32_t shadowMapOffsetX : 10;
		uint32_t shadowMapOffsetY : 10;

		void GetUnpackedShadowMapData( uint32_t& shadowMapScale, uint32_t& shadowMapOffsetX, uint32_t& shadowMapOffsetY ) const;
	};

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
	ALResult UpdateLists( Tr2RenderContext& renderContext );
	void SetVariableStore();
	void AdjustLightCutoff( float lodFactor );

	virtual void ReleaseResources( TriStorage s );

	static void ResetVariableStore();
	static Tr2LightManager* GetOrCreateInstance( const char* effectPath );
	static Tr2LightManager* GetInstance();
	static void DeleteInstance();

	static bool AreLightFlagsValid( uint16_t flags );

	static Tr2TextureArray& GetLightProfileArray();

	const std::vector<uint32_t>& GetShadowCastingLights() const;
	Tr2LightManager::PerLightData& GetLightData( uint32_t index );
	Tr2DepthStencilPtr GetShadowMapAtlas();

private:
	Tr2LightManager( const Tr2LightManager& );
	Tr2LightManager& operator=( const Tr2LightManager& );

	virtual bool OnPrepareResources();

	ALResult DoUpdateLists( Tr2RenderContext& renderContext );
	ALResult ClearLightIndices( Tr2RenderContext& renderContext );
	ALResult UpdateLightBuffer( Tr2RenderContext& renderContext );

	bool GetShadowMapAtlasEntry( uint32_t lightIndex, uint32_t width, uint32_t height, uint32_t& out_posX, uint32_t& out_posY );
	
	uint32_t InsertShadowMapNode( uint32_t nodeId, uint32_t lightIndex, int32_t width, int32_t height );

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

	Tr2Variable m_shadowMapAtlasVariable;
	Tr2DepthStencilPtr m_shadowMapAtlasDS;
	struct ShadowMapNode
	{
		uint32_t children[2];
		uint32_t lightIndex;
		int x;
		int y;
		int width;
		int height;
	};
	std::vector<ShadowMapNode> m_shadowMapNodes;
};

#endif
