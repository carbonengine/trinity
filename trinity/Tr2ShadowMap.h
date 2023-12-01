////////////////////////////////////////////////////////////
//
//    Created:   October 2022
//    Copyright: CCP 2022
//

#pragma once
#define TR2SHADOWMAP_H

#include "Tr2DeviceResource.h"
#include "ITr2TextureProvider.h"
#include "Tr2DepthStencil.h"
#include "TriFrustumOrtho.h"
#include "Tr2Denoiser.h"
#include "Utilities/BoundingBox.h"

const uint16_t SHADOW_MAP_SIZE = 2048;
const uint8_t SHADOW_FRUSTUM_COUNT = 16;

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
class Tr2ShadowMap : public INotify,
					 public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2ShadowMap( IRoot* lockobj = 0 );
	~Tr2ShadowMap();

	enum ShadowQuality
	{
		DISABLED,
		LOW,
		HIGH
	};

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
	bool OnPrepareResources();

	void ClearVariableStore();

	ShadowMap::SplitSetup SetupShadowSplit( int splitIndex, Matrix invViewTransform, const Vector3 lightDirection, float zNear );

	bool PrepareShadowRendering( Tr2RenderContext& renderContext );
	void BeginShadowRendering( Tr2RenderContext& renderContext, int splitIndex );
	void EndShadowRendering( Tr2RenderContext& renderContext );
	void DrawToShadowMapResult( Tr2RenderContext& renderContext, ITr2TextureProvider* depthMap );
	void SetShadowMap( Tr2RenderTargetPtr shadowMapRenderTarget );

	Tr2DepthStencilPtr GetCascadedShadowMapDS() const;
	Tr2RenderTargetPtr GetCascadedShadowMapRT() const;
	ITr2TextureProvider* GetShadowMap() const;

	const unsigned int GetShadowSplitCount() const;
	const unsigned int GetShadowMapSize() const;
	Tr2EffectPtr GetShadowEffect() const;
	bool GetDebugSplitValue() const;
	void SetNoShadow();

	uint32_t GetDebugColors( int switchCase ) const;

	struct PerSplitData
	{
		Vector4 ShadowMapValues[4]; // x = zFar value[0], y = zFar value[1], z = zFar value[2], w = zFar value[3]..etc

		Matrix ShadowMatrixVal[SHADOW_FRUSTUM_COUNT];

		Vector4 SplitInfo; // x = split count
	};

	PerSplitData m_perSplitData;

private:
	const Vector3 m_unitCube[8] = {
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

	ShadowQuality m_quality;

	void SetHighSettingSplitValues();
	void SetLowSettingSplitValues();
	void CreateShadowMaps();

	// width and height of shadow map
	unsigned int m_size; // texture res
	unsigned int m_width; // splits on x axis 
	unsigned int m_height; // splits on y axis
	unsigned int m_splitCount;
	float m_oldZFar;

	// texture to draw shader on
	Tr2DepthStencilPtr m_cascadedShadowMapDS;
	Tr2RenderTargetPtr m_shadowMapResultRT;
	// White texture for no shadow
	TriTextureResPtr m_whiteTexture;

	// the handle to the depth atlas variable
	TriVariable* m_cascadedShadowMapHandle;

	// denoiser
	Tr2DenoiserPtr m_denoiser;

	// shadow shader
	Tr2EffectPtr m_shadowEffect;

	// the handle to the rt variable
	TriVariable* m_shadowMapHandle;

	float m_splitValues[SHADOW_FRUSTUM_COUNT]; // zFar values

	bool m_debugColorSplit;
	bool m_disableShimmer;
};
TYPEDEF_BLUECLASS( Tr2ShadowMap );