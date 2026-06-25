// Copyright © 2022 CCP ehf.

#pragma once
#define TR2SHADOWMAP_H

#include "Tr2DeviceResource.h"
#include "ITr2TextureProvider.h"
#include "Tr2DepthStencil.h"
#include "TriFrustumOrtho.h"
#include "Tr2Denoiser.h"
#include "Utilities/BoundingBox.h"
#include "Tr2GpuResourcePool.h"

const uint16_t SHADOW_MAP_SIZE = 2048;
const uint8_t SHADOW_FRUSTUM_COUNT = 16;

const float MIN_SHADOW_SPLIT = 25.f;
const float MAX_SHADOW_SPLIT = 1228800.f;

const Vector3 DX_UNIT_CUBE[8] = {
	//The unit cube in DirectX is from( -1, -1, 0 ) to ( 1, 1, 1 )
	Vector3( -1, -1, 0 ), // vertex 0
	Vector3( -1, 1, 0 ), // vertex 1
	Vector3( 1, 1, 0 ), // etc..
	Vector3( 1, -1, 0 ),
	Vector3( -1, -1, 1 ),
	Vector3( -1, 1, 1 ),
	Vector3( 1, 1, 1 ),
	Vector3( 1, -1, 1 )
};

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( Tr2GpuStructuredBuffer );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2Denoiser );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE_INTERFACE( ITriTextureRes );

namespace ShadowMap
{
/////////////////////////////////////////////////////////////
// Shadow functions
struct SplitSetup
{
	TriFrustumOrtho shadowFrustum;
	Matrix lightViewProjection;
	Matrix invViewProj;
	AxisAlignedBoundingBox aabb;
};
}
// --------------------------------------------------------------------------------
// Description:
//   This class holds a cascaded shadow map and takes care of splitting the frustum
//
// --------------------------------------------------------------------------------
BLUE_CLASS( Tr2ShadowMap ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2ShadowMap( IRoot* lockobj = 0 );

	void Setup( uint32_t elementSize, uint32_t elementCount, bool useDenoiser );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

	ShadowMap::SplitSetup SetupShadowSplit( int splitIndex, Matrix invViewTransform, const Vector3 lightDirection, float zNear, float leftDivNear, float rightDivNear, float topDivNear, float bottomDivNear );

	Tr2GpuResourcePool::Texture PrepareShadowRendering( Tr2GpuResourcePool & gpuResourcePool, Tr2RenderContext & renderContext );
	void BeginShadowRendering( Tr2RenderContext & renderContext, int splitIndex );
	void EndShadowRendering( Tr2RenderContext & renderContext );
	Tr2GpuResourcePool::Texture DrawToShadowMapResult( Tr2RenderContext & renderContext, Tr2GpuResourcePool & gpuResourcePool, const Tr2TextureAL& depthMap, const Tr2TextureAL& cascadedShadowDepth, float upscaling );

	const unsigned int GetShadowSplitCount() const;
	const unsigned int GetShadowMapSize() const;
	Tr2EffectPtr GetShadowEffect() const;
	bool GetDebugSplitValue() const;

	uint32_t GetDebugColors( int switchCase ) const;

	void ShouldUseDenoiser( bool value );

	void UpdateSplitValues( float nearClip, float farClip );

	struct PerSplitData
	{
		Vector4 ShadowMapValues[4]; // x = zFar value[0], y = zFar value[1], z = zFar value[2], w = zFar value[3]..etc

		Matrix ShadowMatrixVal[SHADOW_FRUSTUM_COUNT];

		Vector4 CascadeRanges[SHADOW_FRUSTUM_COUNT];

		Vector4 SplitInfo; // x = split count
	};

	PerSplitData m_perSplitData;

	enum ShadowSplitMode
	{
		STATIC,
		DYNAMIC,
		MANUAL
	};
	static AxisAlignedBoundingBox CalculateAABB( Matrix projection, Matrix invViewTransform, Matrix lightView, Vector3( &corners )[8] );

private:
private:
	void SetStaticShadowSplits();

	// width and height of shadow map
	unsigned int m_size; // texture res
	unsigned int m_width; // splits on x axis
	unsigned int m_height; // splits on y axis
	unsigned int m_splitCount;
	float m_oldZFar;

	// denoiser
	Tr2DenoiserPtr m_denoiser;
	bool m_useDenoiser;

	// shadow shader
	Tr2EffectPtr m_shadowEffect;

	float m_splitValues[SHADOW_FRUSTUM_COUNT]; // zFar values

	bool m_debugColorSplit;
	bool m_disableShimmer;

	ShadowSplitMode m_shadowSplitMode;
	float m_lastNearClip;
	float m_lastFarClip;
};
TYPEDEF_BLUECLASS( Tr2ShadowMap );
