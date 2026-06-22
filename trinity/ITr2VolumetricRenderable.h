// Copyright © 2023 CCP ehf.

#pragma once

class TriFrustum;
class ITriRenderBatchAccumulator;

#include "Eve/EveComponentRegistry.h"
#include "TriFrustumOrtho.h"

enum class Tr2VolumerticQuality
{
	Low,
	Medium,
	High,
	Ultra,
};
extern const Be::VarChooser Tr2VolumetricQuality_Chooser[];

BLUE_INTERFACE( ITr2VolumetricRenderable ) :
	public IRoot
{
public:
	struct SceneInformation
	{
		static const size_t depthSliceCount = 4;
		Tr2VolumerticQuality quality;
		float depthSlices[depthSliceCount];
		uint32_t targetWidth;
		uint32_t targetHeight;
		Vector3 sunDirection;
		bool receiveShadows;
		bool castShadows;
		bool raytracedShadows;
	};

	struct ShadowInfo
	{
		TriFrustumOrtho shadowFrustum;
		Matrix lightViewProj;
		Vector3 aabbMax;
		uint32_t shadowMapSize;
	};

	virtual float GetSortValue( const TriFrustum& frustum ) = 0;
	virtual void GetVolumetricBatches( const TriFrustum& frustum, ITriRenderBatchAccumulator* batches ) = 0;
	virtual bool UpdateVolumetricLightmap( Tr2RenderContext & renderContext ) = 0;
	virtual void SetSceneInformation( const SceneInformation& sceneInformation ) = 0;
	virtual void GetVolumetricShadowBatches( ITriRenderBatchAccumulator * batches ) = 0;
	virtual void GetVolumetricShadowInfo( ShadowInfo & shadowInfo, Vector3 sunDir ) = 0;
	virtual bool PrepareCloudShadowMap( Tr2RenderContext & renderContext ) = 0;
	virtual void SetCloudShadowMapHandle() = 0;
};

REGISTER_COMPONENT_TYPE( "VolumetricRenderable", ITr2VolumetricRenderable )
